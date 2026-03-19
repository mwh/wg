// Each eval function has the signature:
//   evalNode(node, ctx, cont) => PendingStep | null

import {
    Context, UserObject, GraceNumber, GraceString, GraceBoolean, GraceRange,
    GraceMatchResult, GracePatternOr, GraceTypeRef, GraceLineup,
    GRACE_DONE, GRACE_UNINITIALISED,
    GraceError, pending, nativeMethod, graceMethod, assertString
} from './runtime.js';

// Evaluate a list of AST nodes in sequence, returning the last value.
function evalSequence(nodes, ctx, cont) {
    if (!nodes || nodes.length === 0) {
        return pending(ctx, cont, GRACE_DONE);
    }
    const [first, ...rest] = nodes;
    if (rest.length === 0) {
        return evalNode(first, ctx, cont);
    }
    return evalNode(first, ctx, (_) => evalSequence(rest, ctx, cont));
}

function evalConsList(list, ctx, cont) {
    return evalSequence(consToArray(list), ctx, cont);
}

function consToArray(list) {
    const arr = [];
    let cur = list;
    while (cur && cur.kind === 'cons') {
        arr.push(cur.head);
        cur = cur.tail;
    }
    return arr;
}

//  Evaluate args list: array of AST nodes → array of GraceObjects 
function evalArgs(argNodes, ctx, cont) {
    if (argNodes.length === 0) return cont([]);
    const results = new Array(argNodes.length);
    function step(i) {
        if (i === argNodes.length) return cont(results);
        return evalNode(argNodes[i], ctx, (val) => {
            results[i] = val;
            return step(i + 1);
        });
    }
    return step(0);
}

//  Pre-scan an object body for declarations 
//
// ObjCons pre-populates slots (var, def, type, method) before evaluating
// the body so that forward references (especially recursive methods) work.

function prescanBody(bodyNodes, obj) {
    for (const node of bodyNodes) {
        switch (node.kind) {
            case 'varDec':
                obj.addVar(node.name);
                break;
            case 'defDec':
                obj.addDef(node.name);
                break;
            case 'typeDec':
                obj.addTypeDef(node.name);
                break;
            case 'methDec': {
                const method = buildGraceMethod(node);
                const mname = methodDeclName(node);
                obj.addMethod(mname, method);
                break;
            }
            case 'importStmt':
                obj.addDef(node.asName);
                break;
            case 'comment':
            case 'dialectStmt':
                break;
            default:
                // Expressions, assignments — no slot to pre-create
                break;
        }
    }
}

/**
 * Compute the full multi-part method name for a methDec node.
 * E.g., parts=[{name:"if", params:[p]}, {name:"then", params:[q]}] → "if(1)then(1)"
 */
function methodDeclName(node) {
    const parts = consToArray(node.parts);
    return parts.map(p => {
        const params = consToArray(p.args);
        return params.length === 0 ? p.name : `${p.name}(${params.length})`;
    }).join('');
}

/**
 * Build a graceMethod from a methDec AST node.
 */
function buildGraceMethod(node) {
    const parts = consToArray(node.parts);
    const paramNames = [];
    const genericParamNames = [];
    for (const p of parts) {
        for (const idDecl of consToArray(p.args)) {
            paramNames.push(idDecl.name);
        }
        if (p.genericArgs) {
            for (const g of consToArray(p.genericArgs)) {
                genericParamNames.push(typeof g === 'string' ? g : g.name || g);
            }
        }
    }
    // Collect var/def names from body for pre-allocation
    const bodyNodes = consToArray(node.body);
    const varNames = [];
    const defNames = [];
    for (const stmt of bodyNodes) {
        if (stmt.kind === 'varDec') varNames.push(stmt.name);
        else if (stmt.kind === 'defDec') defNames.push(stmt.name);
    }
    const label = methodDeclName(node);
    function bodyCPS(ctx, cont) {
        return evalConsList(node.body, ctx, cont);
    }
    return graceMethod(paramNames, genericParamNames, bodyCPS, varNames, defNames, true, label);
}

/**
 * Build an inheritable variant of a method whose last body statement is an objCons.
 * The variant has one extra parameter (the inheriting object), and evaluates
 * the objCons body into that object instead of creating a fresh one.
 */
function buildInheritableMethod(node) {
    const parts = consToArray(node.parts);
    const paramNames = [];
    const genericParamNames = [];
    for (const p of parts) {
        for (const idDecl of consToArray(p.args)) {
            paramNames.push(idDecl.name);
        }
        if (p.genericArgs) {
            for (const g of consToArray(p.genericArgs)) {
                genericParamNames.push(typeof g === 'string' ? g : g.name || g);
            }
        }
    }
    const bodyNodes = consToArray(node.body);
    const stmtsBefore = bodyNodes.slice(0, -1);
    const lastObjCons = bodyNodes[bodyNodes.length - 1];
    const varNames = [];
    const defNames = [];
    for (const stmt of stmtsBefore) {
        if (stmt.kind === 'varDec') varNames.push(stmt.name);
        else if (stmt.kind === 'defDec') defNames.push(stmt.name);
    }
    const label = methodDeclName(node) + 'inherit(1)';
    return nativeMethod((ctx, cont, self, args, genericArgs = []) => {
        const inheritObj = args[args.length - 1];
        const regularArgs = args.slice(0, -1);
        let bodyCtx = ctx.withSelf(self);
        bodyCtx = bodyCtx.withReturnContinuation(cont);
        const extCtx = bodyCtx.extendScope(label);
        const localScope = extCtx.scope;
        for (let i = 0; i < paramNames.length; i++) {
            extCtx.bindLocalName(paramNames[i],
                i < regularArgs.length ? regularArgs[i] : GRACE_DONE);
        }
        for (let i = 0; i < genericParamNames.length; i++) {
            const val = i < genericArgs.length ? genericArgs[i] : new UserObject();
            extCtx.bindLocalName(genericParamNames[i], val);
        }
        for (const v of varNames) localScope.addVar(v);
        for (const d of defNames) localScope.addDef(d);
        if (stmtsBefore.length === 0) {
            return evalObjCons(lastObjCons, extCtx, cont, inheritObj);
        }
        return evalSequence(stmtsBefore, extCtx, (_) => {
            return evalObjCons(lastObjCons, extCtx, cont, inheritObj);
        });
    });
}

// Main eval dispatch
export function evalNode(node, ctx, cont) {
    if (!node) return pending(ctx, cont, GRACE_DONE);

    switch (node.kind) {
        case 'numLit':
            return pending(ctx, cont, new GraceNumber(node.value));

        case 'strLit':
            return pending(ctx, cont, new GraceString(node.value));

        case 'comment':
        case 'dialectStmt':
            return pending(ctx, cont, GRACE_DONE);

        case 'nil':
            // Nil in value position means no value (e.g. uninitialized var)
            return pending(ctx, cont, GRACE_DONE);

        case 'cons':
            // A single-element cons list in expression position — unwrap the head
            // (this matches the Java Converter's "cons" case which just returns head)
            return evalNode(node.head, ctx, cont);

        case 'interpStr':
            return evalInterpStr(node, ctx, cont);

        case 'lexReq':
            return evalLexReq(node, ctx, cont);

        case 'dotReq':
            return evalDotReq(node, ctx, cont);

        case 'objCons':
            return evalObjCons(node, ctx, cont);

        case 'block':
            return evalBlock(node, ctx, cont);

        case 'lineup':
            return evalLineup(node, ctx, cont);

        case 'methDec':
            // Method declarations are handled during pre-scan; toCPS returns done
            return pending(ctx, cont, GRACE_DONE);

        case 'defDec':
            return evalDefDec(node, ctx, cont);

        case 'varDec':
            return evalVarDec(node, ctx, cont);

        case 'typeDec':
            return evalTypeDec(node, ctx, cont);

        case 'assn':
            return evalAssn(node, ctx, cont);

        case 'returnStmt':
            return evalReturn(node, ctx, cont);

        case 'importStmt':
            return evalImport(node, ctx, cont);

        default:
            throw new GraceError(`Unknown AST node kind: "${node.kind}"`);
    }
}

//  Node evaluators
function evalInterpStr(node, ctx, cont) {
    // node: { pre, expr, post }
    // post is either a string, a strLit AST node, or another interpStr node
    return evalNode(node.expr, ctx, (exprVal) => {
        return exprVal.requestMethod(ctx, (strVal) => {
            const mid = strVal instanceof GraceString ? strVal.value : strVal.toString();
            const combined = node.pre + mid;
            if (typeof node.post === 'string') {
                // end of interpolation chain — post is the trailing literal (plain string)
                return pending(ctx, cont, new GraceString(combined + node.post));
            }
            if (node.post && node.post.kind === 'strLit') {
                // end of interpolation chain — post is a strLit AST node
                return pending(ctx, cont, new GraceString(combined + node.post.value));
            }
            // post is another interpStr node — recurse, accumulating prefix
            const nextNode = { ...node.post, pre: combined + (node.post.pre || '') };
            return evalNode(nextNode, ctx, cont);
        }, 'asString', []);
    });
}

function evalLexReq(node, ctx, cont) {
    const name = node.name;
    // This is a hack
    if (name === 'true')  return pending(ctx, cont, GraceBoolean.TRUE);
    if (name === 'false') return pending(ctx, cont, GraceBoolean.FALSE);
    if (name === 'done')  return pending(ctx, cont, GRACE_DONE);
    if (name === 'self')  return pending(ctx, cont, ctx.self || GRACE_DONE);

    const argNodes = consToArray(node.args);
    const genericArgNodes = node.genericArgs ? consToArray(node.genericArgs) : [];

    return evalArgs(argNodes, ctx, (args) => {
        return evalArgs(genericArgNodes, ctx, (genericArgs) => {
            // Find receiver in lexical scope chain
            const receiver = ctx.findReceiver(name);
            if (!receiver) {
                throw new GraceError(
                    `Undefined lexical name "${name}"`,
                    ctx.callStack
                );
            }
            return receiver.requestMethod(ctx, cont, name, args, genericArgs);
        });
    });
}

function evalDotReq(node, ctx, cont) {
    const name = node.name;
    const argNodes = consToArray(node.args);
    const genericArgNodes = node.genericArgs ? consToArray(node.genericArgs) : [];

    return evalNode(node.receiver, ctx, (recv) => {
        return evalArgs(argNodes, ctx, (args) => {
            return evalArgs(genericArgNodes, ctx, (genericArgs) => {
                return recv.requestMethod(ctx, cont, name, args, genericArgs);
            });
        });
    });
}

function evalObjCons(node, ctx, cont) {
    const obj = new UserObject();
    obj._debugLabel = 'object';
    // New object's surrounding is the current scope
    obj._surrounding = ctx.scope;
    const bodyNodes = consToArray(node.body);
    // Pre-scan: create slots for all declared names (including methods)
    prescanBody(bodyNodes, obj);
    // Now run the body in the context of the new object
    const innerCtx = ctx.withSelfScope(obj);
    return evalConsList(node.body, innerCtx, (result) => {
        return pending(ctx, cont, obj);
    });
}

function evalBlock(node, ctx, cont) {
    const paramDecls = consToArray(node.params);
    const paramNames = paramDecls.map(p => p.name || p);
    const capturedReturnCont = ctx.returnCont;
    // A block is a UserObject with apply methods
    const blockObj = new UserObject();
    blockObj._debugLabel = 'block';
    blockObj._surrounding = ctx.scope;

    // Pre-scan block body for var/def declarations (not nested blocks)
    const blockBodyNodes = consToArray(node.body);
    const blockVarNames = [];
    const blockDefNames = [];
    for (const stmt of blockBodyNodes) {
        if (stmt.kind === 'varDec') blockVarNames.push(stmt.name);
        else if (stmt.kind === 'defDec') blockDefNames.push(stmt.name);
    }

    function applyBlock(invCtx, applyCont, args) {
        // Use creation-time context for scope chain and returnCont, but inherit
        // dynamic-extent properties (reset prompt, exception handler) from the
        // invocation context so that try/catch and shift/reset work correctly
        // when blocks are applied across those boundaries.
        let innerCtx = ctx.withReturnContinuation(capturedReturnCont).extendScope('block');
        if (invCtx._resetPromptTarget) {
            innerCtx = new Context({ ...innerCtx, _resetPromptTarget: invCtx._resetPromptTarget });
        }
        if (invCtx.exceptionCont) {
            innerCtx = new Context({ ...innerCtx, exceptionCont: invCtx.exceptionCont });
        }
        for (let i = 0; i < paramNames.length; i++) {
            innerCtx.bindLocalName(paramNames[i], i < args.length ? args[i] : GRACE_DONE);
        }
        for (const v of blockVarNames) innerCtx.scope.addVar(v);
        for (const d of blockDefNames) innerCtx.scope.addDef(d);
        return evalConsList(node.body, innerCtx, applyCont);
    }

    blockObj.addMethod('apply', nativeMethod((invCtx, bCont, _self, _args) =>
        applyBlock(invCtx, bCont, [])));

    for (let n = 1; n <= Math.max(paramNames.length, 1); n++) {
        blockObj.addMethod(`apply(${n})`, nativeMethod((invCtx, bCont, _self, args) =>
            applyBlock(invCtx, bCont, args)));
    }

    // Single-param blocks can be used as patterns
    if (paramNames.length === 1) {
        blockObj.addMethod('match(1)', nativeMethod((invCtx, bCont, _self, args) => {
            const target = args[0];
            return applyBlock(invCtx, (result) => {
                let succeeded;
                if (result instanceof GraceBoolean) succeeded = result.value;
                else succeeded = result !== GRACE_DONE;
                return pending(ctx, bCont, new GraceMatchResult(succeeded, target));
            }, [target]);
        }));
    }

    return pending(ctx, cont, blockObj);
}

function evalLineup(node, ctx, cont) {
    const elementNodes = consToArray(node.elements);
    const results = [];
    function step(i) {
        if (i >= elementNodes.length) return pending(ctx, cont, new GraceLineup(results));
        return evalNode(elementNodes[i], ctx, (val) => {
            results.push(val);
            return step(i + 1);
        });
    }
    return step(0);
}

function evalDefDec(node, ctx, cont) {
    return evalNode(node.value, ctx, (val) => {
        // Find the def setter in scope (created during prescan)
        const receiver = ctx.findReceiver(node.name + ' =(1)');
        if (receiver) {
            return receiver.requestMethod(ctx, cont, node.name + ' =(1)', [val]);
        }
        // Fallback: bind directly in current scope
        ctx.bindLocalName(node.name, val);
        return pending(ctx, cont, GRACE_DONE);
    });
}

function evalVarDec(node, ctx, cont) {
    const initNode = node.value && node.value.kind !== 'nil' ? node.value : null;
    if (!initNode) {
        return pending(ctx, cont, GRACE_DONE);
    }
    return evalNode(initNode, ctx, (val) => {
        const receiver = ctx.findReceiver(node.name + ':=(1)');
        if (receiver) {
            return receiver.requestMethod(ctx, cont, node.name + ':=(1)', [val]);
        }
        return pending(ctx, cont, GRACE_DONE);
    });
}

function evalTypeDec(node, ctx, cont) {
    if (!node.typeExpr || node.typeExpr.kind === 'nil') {
        return pending(ctx, cont, GRACE_DONE);
    }
    return evalNode(node.typeExpr, ctx, (typeVal) => {
        const receiver = ctx.findReceiver(node.name + ' =(1)');
        if (receiver) {
            return receiver.requestMethod(ctx, cont, node.name + ' =(1)', [typeVal]);
        }
        return pending(ctx, cont, GRACE_DONE);
    });
}

function evalAssn(node, ctx, cont) {
    // left is a lexReq or dotReq
    const left = node.left;
    if (left.kind === 'lexReq') {
        const setterName = left.name + ':=(1)';
        return evalNode(node.right, ctx, (rhs) => {
            const receiver = ctx.findReceiver(setterName);
            if (!receiver) throw new GraceError(`Cannot assign to "${left.name}" — no setter found`);
            return receiver.requestMethod(ctx, cont, setterName, [rhs]);
        });
    }
    if (left.kind === 'dotReq') {
        return evalNode(left.receiver, ctx, (recv) => {
            return evalNode(node.right, ctx, (rhs) => {
                const setterName = left.name + ':=(1)';
                return recv.requestMethod(ctx, cont, setterName, [rhs]);
            });
        });
    }
    throw new GraceError(`Invalid assignment target: ${left.kind}`);
}

function evalReturn(node, ctx, cont) {
    return evalNode(node.value, ctx, (val) => {
        if (!ctx.returnCont) {
            throw new GraceError('return statement outside of a method');
        }
        return pending(ctx, ctx.returnCont, val);
    });
}

function evalImport(node, ctx, cont) {
    const moduleName = node.name;
    const asName = node.asName;
    // Module registry is set on the context
    const moduleRegistry = ctx._moduleRegistry;
    if (!moduleRegistry) throw new GraceError('No module registry on context');
    const moduleLoader = moduleRegistry.get(moduleName);
    if (!moduleLoader) throw new GraceError(`Unknown module: "${moduleName}"`);
    // Run the module in prelude scope, get back a Grace object
    return moduleLoader(ctx, (moduleObj) => {
        const receiver = ctx.findReceiver(asName + ' =(1)');
        if (receiver) {
            return receiver.requestMethod(ctx, cont, asName + ' =(1)', [moduleObj]);
        }
        ctx.bindLocalName(asName, moduleObj);
        return pending(ctx, cont, GRACE_DONE);
    });
}

//  Module runner 

/**
 * Execute a module (an objCons AST node) in the given prelude context.
 * Returns the resulting UserObject via the continuation.
 */
export function runModule(moduleAst, preludeCtx, cont) {
    return evalNode(moduleAst, preludeCtx, cont);
}

/**
 * Evaluate parser output (a concise-encoding string) using eval(),
 * with the concise builder functions in scope.
 *
 * The string must be a valid JS expression using the concise builders.
 */
export function evalConciseString(code, builders) {
    // Construct a function that has all builders in scope
    const names = Object.keys(builders);
    const values = names.map(k => builders[k]);
    // Using Function constructor (not eval) to limit scope to only builders
    // This is intentional: we only evaluate trusted parser output, not user code.
    const fn = new Function(...names, `return (${code})`);
    return fn(...values);
}
