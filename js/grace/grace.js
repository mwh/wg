import { conciseBuilders } from './concise.js';
import {
    Context, UserObject, GraceString, GraceNumber, GraceBoolean,
    GRACE_DONE, GraceError, pending, runTrampoline
} from './runtime.js';
import { evalNode, runModule, evalConciseString } from './evaluate.js';
import { makePrelude, makeInitialContext } from './prelude.js';

import * as raw from '../modules.js';

// Module source data
//
// The raw AST objects for ast, lexer, parser modules. We lazy-load them by
// importing modules.js which uses the concise builder functions.

let _rawModules = null;

// Interpreter state

/**
 * A fully initialised interpreter, ready to run Grace programs.
 */
export class GraceInterpreter {
    constructor({ print = console.log, readFile = null } = {}) {
        this._print = print;
        this._readFile = readFile;
        this._prelude = null;
        this._ctx = null;
        this._parser = null;   // live Grace parser object
        this._ast = null;      // live Grace ast module object
        this._ready = false;
    }

    async init() {
        if (this._ready) return this;

        const prelude = makePrelude({ print: this._print, readFile: this._readFile });
        const moduleRegistry = new Map();
        const ctx = makeInitialContext(prelude, moduleRegistry);

        // Build a lazy module loader factory
        // Each module is loaded once and cached
        const moduleCache = new Map();
        function makeLoader(astNode) {
            return (loadCtx, loadCont) => {
                if (moduleCache.has(astNode)) {
                    return pending(loadCtx, loadCont, moduleCache.get(astNode));
                }
                return runModule(astNode, ctx, (obj) => {
                    moduleCache.set(astNode, obj);
                    return pending(loadCtx, loadCont, obj);
                });
            };
        }

        moduleRegistry.set('ast',    makeLoader(raw.ast));
        moduleRegistry.set('lexer',  makeLoader(raw.lexer));
        moduleRegistry.set('parser', makeLoader(raw.parser));

        let astModule, lexerModule, parserModule;
        await runTrampoline(
            runModule(raw.ast, ctx, (obj) => { astModule = obj; return null; })
        );
        moduleCache.set(raw.ast, astModule);

        await runTrampoline(
            runModule(raw.lexer, ctx, (obj) => { lexerModule = obj; return null; })
        );
        moduleCache.set(raw.lexer, lexerModule);

        await runTrampoline(
            runModule(raw.parser, ctx, (obj) => { parserModule = obj; return null; })
        );
        moduleCache.set(raw.parser, parserModule);

        this._prelude = prelude;
        this._ctx = ctx;
        this._parser = parserModule;
        this._ast = astModule;
        this._moduleRegistry = moduleRegistry;
        this._moduleCache = moduleCache;
        this._rawModules = raw;
        this._ready = true;
        return this;
    }

    /**
     * Parse a Grace source string using the Grace parser.
     * Returns the concise-encoding string for the parsed AST.
     */
    async parseToString(source, moduleName = 'input') {
        if (!this._ready) await this.init();
        let conciseStr = '';
        await runTrampoline(
            this._parser.requestMethod(this._ctx, (parseResult) => {
                // Call concise() on the result to get a string
                return parseResult.requestMethod(this._ctx, (strObj) => {
                    conciseStr = strObj.toString();
                    return null;
                }, 'concise', []);
            }, 'parseModule(2)', [new GraceString(moduleName), new GraceString(source)])
        );
        return conciseStr;
    }

    /**
     * Parse a Grace source string using the Grace parser.
     * Returns the long-encoding string for the parsed AST.
     */
    async parseToLong(source, moduleName = 'input') {
        if (!this._ready) await this.init();
        let longStr = '';
        await runTrampoline(
            this._parser.requestMethod(this._ctx, (parseResult) => {
                // Call asString() on the result to get a string
                return parseResult.requestMethod(this._ctx, (strObj) => {
                    longStr = strObj.toString();
                    return null;
                }, 'asString', []);
            }, 'parseModule(2)', [new GraceString(moduleName), new GraceString(source)])
        );
        return longStr;
    }

    /**
     * Parse a Grace source string and return the AST as a JS object
     * (using the concise builder eval trick).
     */
    async parse(source, moduleName = 'input') {
        const conciseStr = await this.parseToString(source, moduleName);
        return evalConciseString(conciseStr, conciseBuilders);
    }

    /**
     * Run a Grace source string.
     * Parses the source using the Grace parser, then evaluates the result.
     */
    async runString(source, moduleName = 'input') {
        if (!this._ready) await this.init();
        const ast = await this.parse(source, moduleName);
        await runTrampoline(
            evalNode(ast, this._ctx, (_) => null)
        );
    }

    async runConcise(ast, moduleName = 'input') {
        if (!this._ready) await this.init();
        let nodes = evalConciseString(ast, conciseBuilders);
        await runTrampoline(
            evalNode(nodes, this._ctx, (_) => null)
        );
    }

    /**
     * Run a Grace source file (Node.js only).
     */
    async runFile(filePath) {
        if (!this._readFile) {
            throw new GraceError('runFile requires a readFile function (Node.js only)');
        }
        const src = await this._readFile(filePath);
        await this.runString(src, filePath);
    }
}

// Convenience top-level functions

let _defaultInterpreter = null;

function getDefaultInterpreter() {
    if (!_defaultInterpreter) {
        const opts = {};
        if (typeof process !== 'undefined' && process.versions && process.versions.node) {
            opts.readFile = async (p) => {
                const { readFile } = await import('node:fs/promises');
                return readFile(p, 'utf8');
            };
        }
        _defaultInterpreter = new GraceInterpreter(opts);
    }
    return _defaultInterpreter;
}

export async function runString(source, moduleName) {
    return getDefaultInterpreter().runString(source, moduleName);
}

export async function runFile(filePath) {
    return getDefaultInterpreter().runFile(filePath);
}

export async function runConcise(ast, moduleName) {
    return getDefaultInterpreter().runConcise(ast, moduleName);
}

export async function parse(source, moduleName) {
    return getDefaultInterpreter().parse(source, moduleName);
}

// Re-export key types for consumer use
export {
    GraceString, GraceNumber, GraceBoolean, GRACE_DONE,
    GraceError, conciseBuilders,
    evalNode, evalConciseString, makePrelude
};
