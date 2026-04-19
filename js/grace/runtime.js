/**
 * Core object system and execution engine
 *
 * Architecture:
 *  - All Grace computation is expressed as continuations: (value) => PendingStep | null
 *  - A PendingStep is a thunk (trampoline step): { go() }
 *  - Continuations are plain JS functions and can be called multiple times
 *  - Native methods can use async/await via asyncNativeMethod()
 *  - Trampoline loop yields to the event loop every N steps to keep browser responsive
 */

// Sentinel objects

export const GRACE_DONE = Object.freeze({
    _kind: 'done',
    requestMethod(_ctx, _cont, name, _args) {
        throw new GraceError(`Cannot call method "${name}" on done`);
    },
    toString() { return 'done'; },
});

export const GRACE_UNINITIALISED = Object.freeze({
    _kind: 'uninitialised',
    requestMethod(_ctx, _cont, name, _args) {
        throw new GraceError(`Cannot call method "${name}" on uninitialised variable`);
    },
    toString() { return '<uninitialised>'; },
});

// Error type

export class GraceError extends Error {
    constructor(message, graceCallStack = null) {
        super(message);
        this.name = 'GraceError';
        this.graceCallStack = graceCallStack;
    }
}

export class Context {
    constructor({ self = null, scope = null, returnCont = null, exceptionCont = null, callStack = null, _moduleRegistry = null, _resetPromptTarget = null } = {}) {
        this.self = self;
        this.scope = scope;
        this.returnCont = returnCont;
        this.exceptionCont = exceptionCont;
        this.callStack = callStack;  // linked list: { name, caller }
        this._moduleRegistry = _moduleRegistry;
        // Mutable cell shared by all contexts in the same reset scope.
        // _resetPromptTarget[0] is the continuation the prompt forwards to.
        // shift reads it to find the abort target; k.apply mutates it to redirect.
        this._resetPromptTarget = _resetPromptTarget;
    }

    withSelf(newSelf) {
        return new Context({ ...this, self: newSelf });
    }

    withScope(newScope) {
        return new Context({ ...this, scope: newScope });
    }

    withSelfScope(obj) {
        return new Context({ ...this, self: obj, scope: obj });
    }

    withReturnContinuation(cont) {
        return new Context({ ...this, returnCont: cont });
    }

    withExceptionContinuation(cont) {
        return new Context({ ...this, exceptionCont: cont });
    }

    withResetPromptTarget(cell) {
        return new Context({ ...this, _resetPromptTarget: cell });
    }

    withCall(name, position) {
        const label = position ? `${name} at ${position}` : name;
        return new Context({ ...this, callStack: { name: label, caller: this.callStack } });
    }

    extendScope(label) {
        const newScope = new UserObject();
        if (label) newScope._debugLabel = label;
        if (this.scope) newScope._surrounding = this.scope;
        return new Context({ ...this, scope: newScope });
    }

    bindLocalName(name, value) {
        if (!this.scope) throw new GraceError(`No scope to bind local name ${name}`);
        this.scope.addMethod(name, nativeMethod((_ctx, cont, _self, _args) => {
            return pending(_ctx, cont, value);
        }));
    }

    findReceiver(name) {
        let cur = this.scope;
        while (cur) {
            if (cur.hasMethod(name)) return cur;
            cur = cur._surrounding || null;
        }
        return null;
    }

    getCallStackString() {
        let item = this.callStack;
        let lines = [];
        while (item) {
            lines.push(`  at ${item.name}`);
            item = item.caller;
        }
        return lines.join('\n');
    }
}

//  PendingStep (trampoline)

export function pending(ctx, cont, value) {
    return { go() { return cont(value); }, ctx };
}

// UserObject
export class UserObject {
    constructor() {
        this._methods = Object.create(null);
        this._surrounding = null;
        this._debugLabel = null;
    }

    addMethod(name, method) {
        this._methods[name] = method;
    }

    hasMethod(name) {
        return name in this._methods || name === '==(1)' || name === '!=(1)' || name === 'asString';
    }

    getMethodNames() {
        return Object.keys(this._methods);
    }

    setSurrounding(scope) {
        this._surrounding = scope;
    }

    requestMethod(ctx, cont, name, args, genericArgs = []) {
        const m = this._methods[name];
        if (m) {
            return m.invoke(ctx.withSelfScope(this), cont, this, args, genericArgs);
        }
        // Default methods available on all objects
        switch (name) {
            case '==(1)':  return pending(ctx, cont, GraceBoolean.of(this === args[0]));
            case '!=(1)':  return pending(ctx, cont, GraceBoolean.of(this !== args[0]));
            case 'asString': return pending(ctx, cont, new GraceString(this.toString()));
        }
        const label = this._debugLabel ? `<${this._debugLabel}>` : '<object>';
        const available = Object.keys(this._methods).join(', ');
        throw new GraceError(`No such method "${name}" on ${label}. Available: ${available}`);
    }

    /** Add a mutable variable slot (getter + :=(1) setter) */
    addVar(name) {
        const cell = { value: GRACE_UNINITIALISED };
        this.addMethod(name, nativeMethod((_ctx, cont, _self, _args) => {
            if (cell.value === GRACE_UNINITIALISED) {
                throw new GraceError(`Variable "${name}" is uninitialised`);
            }
            return pending(_ctx, cont, cell.value);
        }));
        this.addMethod(name + ':=(1)', nativeMethod((_ctx, cont, _self, args) => {
            cell.value = args[0];
            return pending(_ctx, cont, GRACE_DONE);
        }));
    }

    /** Add an immutable def slot (getter + internal-setter with space) */
    addDef(name) {
        const cell = { value: GRACE_UNINITIALISED };
        this.addMethod(name, nativeMethod((_ctx, cont, _self, _args) => {
            return pending(_ctx, cont, cell.value);
        }));
        // The internal setter uses an unutterable " =(1)" suffix
        this.addMethod(name + ' =(1)', nativeMethod((_ctx, cont, _self, args) => {
            cell.value = args[0];
            return pending(_ctx, cont, GRACE_DONE);
        }));
    }

    /** Pre-populate type slot with a GraceTypeRef that resolves later */
    addTypeDef(name) {
        const ref = new GraceTypeRef(name);
        this.addMethod(name, nativeMethod((_ctx, cont, _self, _args) => {
            return pending(_ctx, cont, ref);
        }));
        this.addMethod(name + ' =(1)', nativeMethod((_ctx, cont, _self, args) => {
            ref.setType(args[0]);
            return pending(_ctx, cont, GRACE_DONE);
        }));
    }

    toString() {
        return this._debugLabel ? `<${this._debugLabel}>` : '<object>';
    }
}

// GraceTypeRef
export class GraceTypeRef {
    constructor(name) {
        this._name = name;
        this._resolvedType = null;
    }
    setType(type) { this._resolvedType = type; }
    getType() { return this._resolvedType; }

    requestMethod(ctx, cont, name, args, genericArgs = []) {
        if (this._resolvedType) {
            return this._resolvedType.requestMethod(ctx, cont, name, args, genericArgs);
        }
        throw new GraceError(`Type reference "${this._name}" not yet resolved when calling "${name}"`);
    }

    toString() { return `<TypeRef ${this._name}>`; }
}

// Method wrappers

/**
 * A Grace method implemented by a JS function.
 * fn(ctx, cont, self, args, genericArgs) => PendingStep | null
 */
export function nativeMethod(fn) {
    return {
        invoke(ctx, cont, self, args, genericArgs = []) {
            return fn(ctx, cont, self, args, genericArgs);
        }
    };
}

/**
 * A Grace method defined by Grace AST body CPS.
 * paramNames: string[]
 * genericParamNames: string[]
 * bodyCPS: (ctx, cont) => PendingStep | null
 * varNames: string[]
 * defNames: string[]
 * captureReturn: boolean - if true, set up a new return continuation
 */
export function graceMethod(paramNames, genericParamNames, bodyCPS, varNames, defNames, captureReturn, label) {
    return {
        _label: label,
        invoke(ctx, cont, self, args, genericArgs = []) {
            let bodyCtx = ctx.withSelf(self);
            if (captureReturn) {
                // Regular method: 'return' exits this method
                bodyCtx = bodyCtx.withReturnContinuation(cont);
            }
            // If !captureReturn (block apply): returnCont passes through from outer context
            const extCtx = bodyCtx.extendScope(label);
            const localScope = extCtx.scope;
            for (let i = 0; i < paramNames.length; i++) {
                const val = i < args.length ? args[i] : GRACE_DONE;
                extCtx.bindLocalName(paramNames[i], val);
            }
            for (let i = 0; i < genericParamNames.length; i++) {
                const val = i < genericArgs.length ? genericArgs[i] : new UserObject();
                extCtx.bindLocalName(genericParamNames[i], val);
            }
            for (const v of varNames) localScope.addVar(v);
            for (const d of defNames) localScope.addDef(d);
            return bodyCPS(extCtx, cont);
        },
        toString() { return `<Method ${label || '(anonymous)'}>`; }
    };
}

// Primitive objects

export class GraceNumber {
    constructor(value) {
        this.value = value;
    }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'asString': {
                const s = Number.isInteger(this.value) ? String(this.value) : String(this.value);
                return pending(ctx, cont, new GraceString(s));
            }
            case '+(1)': return pending(ctx, cont, new GraceNumber(this.value + assertNumber(args[0]).value));
            case '-(1)': return pending(ctx, cont, new GraceNumber(this.value - assertNumber(args[0]).value));
            case '*(1)': return pending(ctx, cont, new GraceNumber(this.value * assertNumber(args[0]).value));
            case '/(1)': {
                const result = this.value / assertNumber(args[0]).value;
                return pending(ctx, cont, new GraceNumber(result));
            }
            case '%(1)': return pending(ctx, cont, new GraceNumber(this.value % assertNumber(args[0]).value));
            case '==(1)': return pending(ctx, cont, GraceBoolean.of(args[0] instanceof GraceNumber && this.value === assertNumber(args[0]).value));
            case '!=(1)': return pending(ctx, cont, GraceBoolean.of(!(args[0] instanceof GraceNumber) || this.value !== assertNumber(args[0]).value));
            case '<(1)':  return pending(ctx, cont, GraceBoolean.of(this.value <  assertNumber(args[0]).value));
            case '>(1)':  return pending(ctx, cont, GraceBoolean.of(this.value >  assertNumber(args[0]).value));
            case '<=(1)': return pending(ctx, cont, GraceBoolean.of(this.value <= assertNumber(args[0]).value));
            case '>=(1)': return pending(ctx, cont, GraceBoolean.of(this.value >= assertNumber(args[0]).value));
            case '..(1)': return pending(ctx, cont, new GraceRange(this.value, assertNumber(args[0]).value));
            case 'prefix-': return pending(ctx, cont, new GraceNumber(-this.value));
            case 'match(1)': {
                const t = args[0];
                const ok = t instanceof GraceNumber && this.value === t.value;
                return pending(ctx, cont, new GraceMatchResult(ok, t));
            }
            case '|(1)': return pending(ctx, cont, new GracePatternOr(this, args[0]));
            case 'truncated': return pending(ctx, cont, new GraceNumber(Math.trunc(this.value)));
            case 'rounded':   return pending(ctx, cont, new GraceNumber(Math.round(this.value)));
            case 'abs':       return pending(ctx, cont, new GraceNumber(Math.abs(this.value)));
            default:
                throw new GraceError(`No such method "${name}" on Number(${this.value})`);
        }
    }

    hasMethod(name) {
        return [
            'asString', '+(1)', '-(1)', '*(1)', '/(1)', '%(1)',
            '==(1)', '!=(1)', '<(1)', '>(1)', '<=(1)', '>=(1)',
            '..(1)', 'prefix-', 'match(1)', '|(1)', 'truncated', 'rounded', 'abs'
        ].includes(name);
    }

    toString() {
        return Number.isInteger(this.value) ? String(this.value) : String(this.value);
    }
}

export class GraceString {
    constructor(value) {
        this.value = value;
    }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'asString': return pending(ctx, cont, this);
            case 'concise':  return pending(ctx, cont, this);
            case '++(1)': {
                const arg = args[0];
                return arg.requestMethod(ctx, (strObj) => {
                    return pending(ctx, cont, new GraceString(this.value + strObj.toString()));
                }, 'asString', []);
            }
            case 'size': return pending(ctx, cont, new GraceNumber(this.value.length));
            case 'at(1)': {
                const idx = assertNumber(args[0]).value;
                if (idx < 1 || idx > this.value.length) {
                    throw new GraceError(`String index ${idx} out of bounds (size ${this.value.length})`);
                }
                return pending(ctx, cont, new GraceString(this.value[idx - 1]));
            }
            case 'firstCodepoint':
            case 'firstCP': return pending(ctx, cont, new GraceNumber(this.value.codePointAt(0)));
            case '==(1)': return pending(ctx, cont, GraceBoolean.of(this.value === assertString(args[0]).value));
            case '!=(1)': return pending(ctx, cont, GraceBoolean.of(this.value !== assertString(args[0]).value));
            case '<(1)':  return pending(ctx, cont, GraceBoolean.of(this.value <  assertString(args[0]).value));
            case '>(1)':  return pending(ctx, cont, GraceBoolean.of(this.value >  assertString(args[0]).value));
            case 'replace(1)with(1)':
                return pending(ctx, cont, new GraceString(
                    this.value.replaceAll(assertString(args[0]).value, assertString(args[1]).value)
                ));
            case 'substringFrom(1)to(1)': {
                const s = assertNumber(args[0]).value;
                const e = assertNumber(args[1]).value;
                return pending(ctx, cont, new GraceString(this.value.slice(s - 1, e)));
            }
            case 'trim': return pending(ctx, cont, new GraceString(this.value.trim()));
            case 'match(1)': {
                const t = args[0];
                const ok = t instanceof GraceString && this.value === t.value;
                return pending(ctx, cont, new GraceMatchResult(ok, t));
            }
            case '|(1)': return pending(ctx, cont, new GracePatternOr(this, args[0]));
            default:
                throw new GraceError(`No such method "${name}" on String("${this.value}")`);
        }
    }

    hasMethod(name) {
        return [
            'asString', 'concise', '++(1)', 'size', 'at(1)',
            'firstCodepoint', 'firstCP', '==(1)', '!=(1)', '<(1)', '>(1)',
            'replace(1)with(1)', 'substringFrom(1)to(1)', 'trim', 'match(1)', '|(1)'
        ].includes(name);
    }

    toString() { return this.value; }
}

export class GraceBoolean {
    constructor(value) {
        this.value = value;
    }

    static of(v) { return v ? GraceBoolean.TRUE : GraceBoolean.FALSE; }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'asString': return pending(ctx, cont, new GraceString(this.value ? 'true' : 'false'));
            case 'not':
            case 'prefix!': return pending(ctx, cont, GraceBoolean.of(!this.value));
            case '==(1)': return pending(ctx, cont, GraceBoolean.of(this.value === assertBoolean(args[0]).value));
            case '!=(1)': return pending(ctx, cont, GraceBoolean.of(this.value !== assertBoolean(args[0]).value));
            case '&&(1)': {
                // Short-circuit: args[0] may be a block
                if (!this.value) return pending(ctx, cont, GraceBoolean.FALSE);
                const arg = args[0];
                if (arg instanceof GraceBoolean) return pending(ctx, cont, arg);
                // It's a block - apply it
                return arg.requestMethod(ctx, cont, 'apply', []);
            }
            case '||(1)': {
                if (this.value) return pending(ctx, cont, GraceBoolean.TRUE);
                const arg = args[0];
                if (arg instanceof GraceBoolean) return pending(ctx, cont, arg);
                return arg.requestMethod(ctx, cont, 'apply', []);
            }
            case 'match(1)': {
                const t = args[0];
                const ok = t instanceof GraceBoolean && this.value === t.value;
                return pending(ctx, cont, new GraceMatchResult(ok, t));
            }
            case '|(1)': return pending(ctx, cont, new GracePatternOr(this, args[0]));
            default:
                throw new GraceError(`No such method "${name}" on Boolean(${this.value})`);
        }
    }

    hasMethod(name) {
        return ['asString', 'not', 'prefix!', '==(1)', '!=(1)', '&&(1)', '||(1)', 'match(1)', '|(1)'].includes(name);
    }

    toString() { return this.value ? 'true' : 'false'; }
}
GraceBoolean.TRUE  = new GraceBoolean(true);
GraceBoolean.FALSE = new GraceBoolean(false);

export class GraceLineup {
    constructor(elements) {
        this.elements = elements; // plain JS array of GraceObjects
    }

    _eachHelper(ctx, cont, block, index) {
        if (index >= this.elements.length) return pending(ctx, cont, GRACE_DONE);
        return block.requestMethod(ctx,
            (_) => this._eachHelper(ctx, cont, block, index + 1),
            'apply(1)', [this.elements[index]]);
    }

    _mapHelper(ctx, cont, block, index, results) {
        if (index >= this.elements.length) return pending(ctx, cont, new GraceLineup(results));
        return block.requestMethod(ctx, (result) => {
            results.push(result);
            return this._mapHelper(ctx, cont, block, index + 1, results);
        }, 'apply(1)', [this.elements[index]]);
    }

    _filterHelper(ctx, cont, block, index, results) {
        if (index >= this.elements.length) return pending(ctx, cont, new GraceLineup(results));
        const el = this.elements[index];
        return block.requestMethod(ctx, (test) => {
            if (test instanceof GraceBoolean && test.value) results.push(el);
            return this._filterHelper(ctx, cont, block, index + 1, results);
        }, 'apply(1)', [el]);
    }

    _asStringHelper(ctx, cont, index, sb) {
        if (index >= this.elements.length) {
            return pending(ctx, cont, new GraceString(sb + ']'));
        }
        if (index > 0) sb += ', ';
        return this.elements[index].requestMethod(ctx, (strObj) => {
            return this._asStringHelper(ctx, cont, index + 1, sb + strObj.toString());
        }, 'asString', []);
    }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'do(1)':
            case 'each(1)':
                return this._eachHelper(ctx, cont, args[0], 0);
            case 'map(1)':
                return this._mapHelper(ctx, cont, args[0], 0, []);
            case 'filter(1)':
                return this._filterHelper(ctx, cont, args[0], 0, []);
            case 'size':
                return pending(ctx, cont, new GraceNumber(this.elements.length));
            case 'at(1)': {
                const idx = args[0] instanceof GraceNumber ? args[0].value : Number(args[0]);
                if (idx < 1 || idx > this.elements.length) {
                    throw new GraceError(`Lineup index ${idx} out of bounds (size ${this.elements.length})`);
                }
                return pending(ctx, cont, this.elements[idx - 1]);
            }
            case '++(1)': {
                const other = args[0];
                if (other instanceof GraceLineup) {
                    return pending(ctx, cont, new GraceLineup([...this.elements, ...other.elements]));
                }
                throw new GraceError(`Cannot concatenate Lineup with ${other}`);
            }
            case 'asString':
                return this._asStringHelper(ctx, cont, 0, '[');
            case '==(1)':
                return pending(ctx, cont, GraceBoolean.of(this === args[0]));
            case '!=(1)':
                return pending(ctx, cont, GraceBoolean.of(this !== args[0]));
            default:
                throw new GraceError(`No such method "${name}" on Lineup`);
        }
    }

    hasMethod(name) {
        return ['each(1)', 'do(1)', 'map(1)', 'filter(1)', 'size', 'at(1)', '++(1)',
                'asString', '==(1)', '!=(1)'].includes(name);
    }

    toString() { return `[${this.elements.map(e => e.toString()).join(', ')}]`; }
}

export class GraceRange {
    constructor(start, end, step = 1) {
        this.start = start;
        this.end   = end;
        this.step  = step;
    }

    _each(ctx, cont, fn, current) {
        if ((this.step > 0 && current > this.end) || (this.step < 0 && current < this.end)) {
            return pending(ctx, cont, GRACE_DONE);
        }
        return pending(ctx, (_) =>
            fn.requestMethod(ctx, (_) => this._each(ctx, cont, fn, current + this.step),
                'apply(1)', [new GraceNumber(current)]), null
        );
    }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'asString':
                return pending(ctx, cont, new GraceString(`${this.start}..${this.end}`));
            case 'do(1)':
            case 'each(1)':
                return this._each(ctx, cont, args[0], this.start);
            case '..(1)': {
                // step syntax: range.by(n) via ".." in Grace is actually handled as by(1)
                const s = assertNumber(args[0]).value;
                return pending(ctx, cont, new GraceRange(this.start, this.end, s));
            }
            case 'match(1)': {
                const t = args[0];
                if (t instanceof GraceNumber) {
                    const v = t.value;
                    const inRange = this.step > 0 ? (v >= this.start && v <= this.end) : (v <= this.start && v >= this.end);
                    return pending(ctx, cont, new GraceMatchResult(inRange, t));
                }
                return pending(ctx, cont, new GraceMatchResult(false, t));
            }
            case '|(1)': return pending(ctx, cont, new GracePatternOr(this, args[0]));
            default:
                throw new GraceError(`No such method "${name}" on Range`);
        }
    }

    toString() { return `${this.start}..${this.end}`; }
}

export class GraceMatchResult {
    constructor(succeeded, value) {
        this.succeeded = succeeded;
        this.value = value;
    }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'succeeded': return pending(ctx, cont, GraceBoolean.of(this.succeeded));
            case 'value':     return pending(ctx, cont, this.value);
            case 'ifTrue(1)':
                return this.succeeded
                    ? args[0].requestMethod(ctx, cont, 'apply', [])
                    : pending(ctx, cont, GRACE_DONE);
            case 'ifFalse(1)':
                return !this.succeeded
                    ? args[0].requestMethod(ctx, cont, 'apply', [])
                    : pending(ctx, cont, GRACE_DONE);
            case 'ifTrue(1)ifFalse(1)':
                return this.succeeded
                    ? args[0].requestMethod(ctx, cont, 'apply', [])
                    : args[1].requestMethod(ctx, cont, 'apply', []);
            case 'ifSuccess(1)':
                return this.succeeded
                    ? args[0].requestMethod(ctx, cont, 'apply(1)', [this.value])
                    : pending(ctx, cont, GRACE_DONE);
            case 'ifSuccess(1)otherwise(1)':
                return this.succeeded
                    ? args[0].requestMethod(ctx, cont, 'apply(1)', [this.value])
                    : args[1].requestMethod(ctx, cont, 'apply', []);
            case 'chain(1)':
                return this.succeeded
                    ? args[0].requestMethod(ctx, cont, 'match(1)', [this.value])
                    : pending(ctx, cont, this);
            case 'match(1)': return pending(ctx, cont, this);
            case 'prefix!':  return pending(ctx, cont, new GraceMatchResult(!this.succeeded, this.value));
            case 'asString':
                return pending(ctx, cont, new GraceString(this.succeeded ? `MatchResult(succeeded)` : `MatchResult(failed)`));
            default:
                throw new GraceError(`No such method "${name}" on MatchResult`);
        }
    }

    toString() { return this.succeeded ? 'MatchResult(succeeded)' : 'MatchResult(failed)'; }
}

export class GracePatternOr {
    constructor(left, right) {
        this.left = left;
        this.right = right;
    }

    requestMethod(ctx, cont, name, args, _genericArgs = []) {
        switch (name) {
            case 'match(1)': {
                const target = args[0];
                return this.left.requestMethod(ctx, (leftResult) => {
                    const mr = assertMatchResult(leftResult);
                    if (mr.succeeded) return pending(ctx, cont, mr);
                    return this.right.requestMethod(ctx, cont, 'match(1)', [target]);
                }, 'match(1)', [target]);
            }
            case '|(1)': return pending(ctx, cont, new GracePatternOr(this, args[0]));
            case 'asString': return pending(ctx, cont, new GraceString(`(${this.left}|${this.right})`));
            default:
                throw new GraceError(`No such method "${name}" on PatternOr`);
        }
    }

    toString() { return `(${this.left}|${this.right})`; }
}

// Assertion helpers

export function assertNumber(obj) {
    if (obj instanceof GraceNumber) return obj;
    throw new GraceError(`Expected Number, got ${obj}`);
}

export function assertString(obj) {
    if (obj instanceof GraceString) return obj;
    throw new GraceError(`Expected String, got ${obj}`);
}

export function assertBoolean(obj) {
    if (obj instanceof GraceBoolean) return obj;
    throw new GraceError(`Expected Boolean, got ${obj}`);
}

export function assertMatchResult(obj) {
    if (obj instanceof GraceMatchResult) return obj;
    throw new GraceError(`Expected MatchResult, got ${obj}`);
}

// Trampoline runner
//
// Runs the CPS computation to completionm yielding to the event loop every
// YIELD_INTERVAL steps so the browser stays responsive.

const YIELD_INTERVAL = 50_000;

/**
 * Run a pending step trampoline. Returns a Promise that resolves when done.
 * Yields to the event-loop every YIELD_INTERVAL steps.
 */
export async function runTrampoline(initialStep) {
    let step = initialStep;
    let steps = 0;
    while (step !== null && step !== undefined) {
        step = step.go();
        steps++;
        if (steps % YIELD_INTERVAL === 0) {
            await new Promise(resolve => setTimeout(resolve, 0));
        }
    }
}

/**
 * Synchronous trampoline - use only when you know the computation is finite
 * and you don't need browser responsiveness (e.g. during bootstrap).
 */
export function runTrampolineSync(initialStep) {
    let step = initialStep;
    while (step !== null && step !== undefined) {
        step = step.go();
    }
}

/**
 * Wrap an async native implementation so it integrates with the continuation system.
 *
 * The asyncFn receives (ctx, args, genericArgs) and returns a Promise<GraceObject>.
 * The continuation is invoked when the promise resolves.
 * Errors from the promise are routed to the exception continuation if set.
 */
export function asyncNativeMethod(asyncFn) {
    return nativeMethod((ctx, cont, _self, args, genericArgs) => {
        // Schedule the async work; return null to the synchronous trampoline
        // (the async continuation will resume independently)
        asyncFn(ctx, args, genericArgs).then(
            result => runTrampoline(pending(ctx, cont, result)),
            err => {
                if (ctx.exceptionCont) {
                    const exc = new GraceString(String(err));
                    runTrampoline(pending(ctx, ctx.exceptionCont, exc));
                } else {
                    console.error('Unhandled Grace async error:', err);
                }
            }
        );
        return null;
    });
}
