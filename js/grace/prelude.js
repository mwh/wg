import {
    Context, UserObject, GraceNumber, GraceString, GraceBoolean,
    GraceMatchResult, GRACE_DONE, GraceError,
    nativeMethod, pending, runTrampoline, assertString
} from './runtime.js';

/**
 * Create a UserObject representing the prelude — the outermost lexical scope.
 * All top-level Grace programs see these methods.
 *
 * @param {object} options
 * @param {function} options.print - Platform print function (string => void)
 * @param {function?} options.readFile - Platform file reader (path => Promise<string>)
 * @returns {UserObject}
 */
export function makePrelude({ print = console.log, readFile = null } = {}) {
    const prelude = new UserObject();
    prelude._debugLabel = 'prelude';

    // print(1)
    prelude.addMethod('print(1)', nativeMethod((ctx, cont, _self, args) => {
        const arg = args[0];
        return arg.requestMethod(ctx, (strObj) => {
            print(strObj.toString());
            return pending(ctx, cont, GRACE_DONE);
        }, 'asString', []);
    }));

    prelude.addMethod('true', nativeMethod((_ctx, cont, _self, _args) =>
        pending(_ctx, cont, GraceBoolean.TRUE)));
    prelude.addMethod('false', nativeMethod((_ctx, cont, _self, _args) =>
        pending(_ctx, cont, GraceBoolean.FALSE)));
    prelude.addMethod('done', nativeMethod((_ctx, cont, _self, _args) =>
        pending(_ctx, cont, GRACE_DONE)));

    prelude.addMethod('if(1)then(1)', nativeMethod((ctx, cont, _self, args) => {
        const cond = args[0];
        if (!(cond instanceof GraceBoolean)) {
            throw new GraceError(`if condition must be Boolean, got ${cond}`);
        }
        if (cond.value) {
            return args[1].requestMethod(ctx, cont, 'apply', []);
        }
        return pending(ctx, cont, GRACE_DONE);
    }));

    prelude.addMethod('if(1)then(1)else(1)', nativeMethod((ctx, cont, _self, args) => {
        const cond = args[0];
        if (!(cond instanceof GraceBoolean)) {
            throw new GraceError(`if condition must be Boolean, got ${cond}`);
        }
        const branch = cond.value ? args[1] : args[2];
        return branch.requestMethod(ctx, cont, 'apply', []);
    }));

    // if(1)then(1)elseif(N)then(N) chains, with or without else
    // In Grace: if(cond)then(thenBlock)elseif(condBlock)then(elseifBlock)...
    // - args[0]: already-evaluated Boolean (if condition)
    // - args[1]: then block
    // - args[2], args[4], ...: elseif condition blocks
    // - args[3], args[5], ...: elseif then blocks
    // - last arg (if odd arg count): else block
    function makeIfElseifMethod(hasElse) {
        return nativeMethod((ctx, cont, _self, args) => {
            const condition = args[0];
            if (!(condition instanceof GraceBoolean)) {
                throw new GraceError(`if condition must be Boolean, got ${condition}`);
            }
            if (condition.value) {
                return args[1].requestMethod(ctx, cont, 'apply', []);
            }
            // Build a chain of elseif continuations (backwards, like Java)
            const lastElseifIndex = hasElse ? args.length - 3 : args.length - 2;
            let next;
            if (hasElse) {
                const elseBlock = args[args.length - 1];
                next = () => elseBlock.requestMethod(ctx, cont, 'apply', []);
            } else {
                next = () => pending(ctx, cont, GRACE_DONE);
            }
            // Build chain backwards
            for (let i = lastElseifIndex; i >= 2; i -= 2) {
                const elifCondBlock = args[i];
                const elifBlock = args[i + 1];
                const currentNext = next;
                next = () => elifCondBlock.requestMethod(ctx, (elifCondVal) => {
                    if (!(elifCondVal instanceof GraceBoolean)) {
                        throw new GraceError(`elseif condition must be Boolean, got ${elifCondVal}`);
                    }
                    if (elifCondVal.value) {
                        return elifBlock.requestMethod(ctx, cont, 'apply', []);
                    }
                    return currentNext();
                }, 'apply', []);
            }
            return next();
        });
    }

    for (let n = 1; n <= 10; n++) {
        const nameParts = ['if(1)then(1)'];
        for (let i = 0; i < n; i++) nameParts.push('elseif(1)then(1)');
        const nameNoElse = nameParts.join('');
        const nameWithElse = nameNoElse + 'else(1)';
        prelude.addMethod(nameNoElse,   makeIfElseifMethod(false));
        prelude.addMethod(nameWithElse, makeIfElseifMethod(true));
    }

    prelude.addMethod('while(1)do(1)', nativeMethod((ctx, cont, _self, args) => {
        const condBlock = args[0];
        const doBlock = args[1];
        function loop() {
            return condBlock.requestMethod(ctx, (condVal) => {
                if (!(condVal instanceof GraceBoolean)) {
                    throw new GraceError(`while condition must be Boolean`);
                }
                if (!condVal.value) return pending(ctx, cont, GRACE_DONE);
                return doBlock.requestMethod(ctx, (_) => loop(), 'apply', []);
            }, 'apply', []);
        }
        return loop();
    }));

    prelude.addMethod('for(1)do(1)', nativeMethod((ctx, cont, _self, args) => {
        const iterable = args[0];
        const loopBlock = args[1];
        return iterable.requestMethod(ctx, cont, 'do(1)', [loopBlock]);
    }));

    prelude.addMethod('successfulMatch(1)', nativeMethod((ctx, cont, _self, args) => {
        return pending(ctx, cont, new GraceMatchResult(true, args[0]));
    }));
    prelude.addMethod('failedMatch(1)', nativeMethod((ctx, cont, _self, args) => {
        return pending(ctx, cont, new GraceMatchResult(false, args[0]));
    }));

    // match(1)case(N) — up to 30 cases
    for (let n = 1; n <= 30; n++) {
        const name = `match(1)${'case(1)'.repeat(n)}`;
        prelude.addMethod(name, nativeMethod((ctx, cont, _self, args) => {
            const target = args[0];
            const cases = args.slice(1);
            function tryCase(i) {
                if (i >= cases.length) return pending(ctx, cont, GRACE_DONE);
                const caseBlock = cases[i];
                return caseBlock.requestMethod(ctx, (mr) => {
                    if (mr instanceof GraceMatchResult && mr.succeeded) {
                        return caseBlock.requestMethod(ctx, cont, 'apply(1)', [target]);
                    }
                    // caseBlock.match(1)(target) — check via the pattern
                    return tryCase(i + 1);
                }, 'match(1)', [target]);
            }
            function tryApply(i) {
                if (i >= cases.length) return pending(ctx, cont, GRACE_DONE);
                const caseBlock = cases[i];
                // Try calling match(1) on the case block with the target
                if (caseBlock.hasMethod && caseBlock.hasMethod('match(1)')) {
                    return caseBlock.requestMethod(ctx, (mr) => {
                        if (mr instanceof GraceMatchResult && mr.succeeded) {
                            return caseBlock.requestMethod(ctx, cont, 'apply(1)', [mr.value !== undefined ? mr.value : target]);
                        }
                        return tryApply(i + 1);
                    }, 'match(1)', [target]);
                }
                // Fallback: try to apply it directly
                return caseBlock.requestMethod(ctx, cont, 'apply(1)', [target]);
            }
            return tryApply(0);
        }));
    }

    prelude.addMethod('getStackTrace', nativeMethod((ctx, cont, _self, _args) => {
        return pending(ctx, cont, new GraceString(ctx.getCallStackString()));
    }));

    prelude.addMethod('try(1)catch(1)', nativeMethod((ctx, cont, _self, args) => {
        const tryBlock = args[0];
        const catchBlock = args[1];
        const exnCont = (exnValue) =>
            catchBlock.requestMethod(ctx, cont, 'apply(1)', [exnValue]);
        const tryCtx = ctx.withExceptionContinuation(exnCont);
        return tryBlock.requestMethod(tryCtx, cont, 'apply', []);
    }));

    // promptTarget[0] acts as a shared cell starting as the outer continuation of
    // reset. When k.apply(v) is called on a captured continuation, it redirects
    // promptTarget[0] to the apply-caller's continuation before resuming, so
    // the result of k.apply(v) flows back to the caller (enabling multi-shot use).
    prelude.addMethod('reset(1)', nativeMethod((ctx, cont, _self, args) => {
        const bodyBlock = args[0];
        // Mutable cell: promptTarget[0] is where the prompt continuation forwards to.
        const promptTarget = [cont];
        const promptCont = (bodyResult) => promptTarget[0](bodyResult);
        const resetCtx = ctx.withResetPromptTarget(promptTarget);
        return bodyBlock.requestMethod(resetCtx, promptCont, 'apply', []);
    }));

    prelude.addMethod('shift(1)', nativeMethod((ctx, cont, _self, args) => {
        const shiftBody = args[0];
        const promptTarget = ctx._resetPromptTarget;
        if (!promptTarget) {
            throw new GraceError('shift called outside of any reset');
        }
        // Snapshot the current abort target (outer continuation of enclosing reset)
        const shiftAbortTarget = promptTarget[0];
        // The captured continuation from this shift point through to the prompt.
        const capturedK = cont;
        // Build a reified Grace continuation object.
        // When k.apply(v) is called:
        //   - Redirect the prompt to the apply-caller's continuation.
        //   - Resume from the shift point with v; when computation reaches the
        //     prompt it forwards to the apply-caller's continuation, making the
        //     result of k.apply(v) land back at the call site (multi-shot safe).
        const reifiedCont = new UserObject();
        reifiedCont._debugLabel = 'continuation';
        reifiedCont.addMethod('apply(1)', nativeMethod((_applyCtx, applyCont, _s, applyArgs) => {
            const resumeValue = applyArgs[0];
            promptTarget[0] = applyCont;
            return pending(_applyCtx, capturedK, resumeValue);
        }));
        // Invoke the shift body; its result goes directly to shiftAbortTarget
        return shiftBody.requestMethod(ctx, shiftAbortTarget, 'apply(1)', [reifiedCont]);
    }));

    // Exception object
    const exceptionProto = new UserObject();
    exceptionProto._debugLabel = 'Exception';
    exceptionProto.addMethod('message', nativeMethod((ctx, cont, _self, _args) =>
        pending(ctx, cont, new GraceString('An exception occurred'))));
    exceptionProto.addMethod('asString', nativeMethod((ctx, cont, self, _args) =>
        self.requestMethod(ctx, cont, 'message', [])));
    exceptionProto.addMethod('refine(1)', nativeMethod((ctx, cont, _self, args) => {
        const name = args[0];
        const refined = new UserObject();
        refined._debugLabel = `Exception(${name})`;
        refined._surrounding = exceptionProto;
        let nameStr = 'Exception';
        refined.addMethod('message', nativeMethod((ctx2, cont2, _s, _a) =>
            pending(ctx2, cont2, name instanceof GraceString ? name : new GraceString(nameStr))));
        refined.addMethod('asString', nativeMethod((ctx2, cont2, self2, _a) =>
            self2.requestMethod(ctx2, cont2, 'message', [])));
        refined.addMethod('refine(1)', exceptionProto._methods['refine(1)']);
        const typeName = name instanceof GraceString ? name.value : String(name);
        refined.addMethod('raise(1)', makeRaiseMethod(typeName));
        return pending(ctx, cont, refined);
    }));
    exceptionProto.addMethod('raise(1)', makeRaiseMethod('Exception'));
    prelude.addMethod('Exception', nativeMethod((ctx, cont, _self, _args) =>
        pending(ctx, cont, exceptionProto)));

    //  Node.js-only: getFileContents(1)
    if (readFile) {
        prelude.addMethod('getFileContents(1)', nativeMethod((ctx, cont, _self, args) => {
            const pathStr = args[0] instanceof GraceString ? args[0].value : args[0].toString();
            readFile(pathStr).then(
                contents => runTrampoline(pending(ctx, cont, new GraceString(contents))),
                err => { throw new GraceError(`getFileContents: ${err.message}`); }
            );
            return null;
        }));
    }

    return prelude;
}

function makeRaiseMethod(typeName) {
    return nativeMethod((ctx, cont, self, args) => {
        const msg = args[0] instanceof GraceString ? args[0].value : args[0].toString();
        const fullMsg = typeName ? `${typeName}: ${msg}` : msg;
        if (ctx.exceptionCont) {
            const errObj = new UserObject();
            errObj._debugLabel = typeName || 'Exception';
            // e.message returns the raw message; e.asString returns "TypeName: message"
            errObj.addMethod('message', nativeMethod((_c, k, _s, _a) => pending(_c, k, new GraceString(msg))));
            errObj.addMethod('asString', nativeMethod((_c, k, _s, _a) => pending(_c, k, new GraceString(fullMsg))));
            return pending(ctx, ctx.exceptionCont, errObj);
        }
        const err = new GraceError(fullMsg);
        err._isGraceException = true;
        throw err;
    });
}

/**
 * Create the initial Context for running a Grace program.
 * Attaches the prelude as the outermost scope, sets up self, and
 * wires the module registry.
 */
export function makeInitialContext(prelude, moduleRegistry) {
    return new Context({
        self: prelude,
        scope: prelude,
        returnCont: null,
        exceptionCont: null,
        callStack: null,
        _moduleRegistry: moduleRegistry,
    });
}
