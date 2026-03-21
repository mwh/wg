/**
 * Concise Encoding Builder Functions
 *
 * These are the builder functions expected by modules.js (the pre-baked Grace
 * parser/lexer/AST module).  They produce plain JavaScript objects (AST nodes)
 * with a `.kind` property that the CPS evaluator dispatches on.
 *
 * Naming follows the Java ConciseEncoding class:
 *   nil       - empty ConsList
 *   c0N(v,l)  - cons(v, l)         -> { kind:"cons",  head, tail }
 *   o1N(v)    - [v]                -> cons(v, nil)
 *   c2N(a,b)  - [a, b]             -> cons(a, cons(b, nil))
 *   s0L(s)    - StrLit             -> { kind:"strLit", value }
 *   n0M(n)    - NumLit             -> { kind:"numLit", value }
 *   l0R       - LexReq
 *   d0R       - DotReq
 *   p0T       - Part
 *   o0C       - ObjCons
 *   b1K       - Block
 *   v4R       - VarDec
 *   d3F       - DefDec
 *   a5N       - Assn
 *   i0D       - IdentifierDeclaration
 *   i0S       - InterpStr
 *   m0D       - MethodDecl
 *   r3T       - ReturnStmt
 *   c0M       - Comment
 *   i0M       - ImportStmt
 *   t0D       - TypeDec
 *   i0C       - InterfaceCons
 *   m0S       - MethSig
 *   s4F       - string concat (pre + mid + post, assembled at AST build time)
 *
 * Character escape constants (c9X):
 *   c9D=$  c9B=\  c9Q="  c9N=\n  c9R=\r  c9L={  c9S=*  c9T=~
 *   c9G=`  c9C=^  c9A=@  c9P=%  c9M=&   c9H=#  c9E=!
 */

//  List primitives
export const nil = Object.freeze({ kind: 'nil' });

export function c0N(head, tail) {
    return { kind: 'cons', head, tail };
}

export function o1N(v) {
    return c0N(v, nil);
}

export function c2N(a, b) {
    return c0N(a, c0N(b, nil));
}

//  Literal nodes
export function s0L(value) {
    return { kind: 'strLit', value: String(value) };
}

export function n0M(value) {
    return { kind: 'numLit', value: Number(value) };
}

//  Request nodes

// Strip trailing "(0)" from method names (zero-arg methods)
function stripZeroArity(name) {
    if (typeof name === 'string' && name.endsWith('(0)')) {
        return name.slice(0, -3);
    }
    return name;
}

/** LexReq: lexical-scope method request */
export function l0R(name, args, genericArgs = nil) {
    return { kind: 'lexReq', name: stripZeroArity(name), args, genericArgs };
}

/** DotReq: dot-notation method request on explicit receiver */
export function d0R(receiver, name, args, genericArgs = nil) {
    return { kind: 'dotReq', receiver, name: stripZeroArity(name), args, genericArgs };
}

/** Part: one part of a multi-part method */
export function p0T(name, args, genericArgs = nil) {
    return { kind: 'part', name: stripZeroArity(name), args, genericArgs };
}

//  Statement/declaration nodes

/** ObjCons: object constructor */
export function o0C(body, annotations = nil) {
    return { kind: 'objCons', body, annotations };
}

/** Block: block literal {params -> body} */
export function b1K(params, body) {
    return { kind: 'block', params, body };
}

/**
 * VarDec: var declaration
 * name:      string
 * declType:  AST node or nil
 * anns:      ConsList of AST annotation nodes
 * valueExpr: AST node or nil (initial value)
 */
export function v4R(name, declType, anns, valueExpr) {
    return { kind: 'varDec', name, declType, annotations: anns, value: valueExpr };
}

/**
 * DefDec: def declaration
 * name:      string
 * declType:  AST node or nil
 * anns:      ConsList of string annotations
 * valueExpr: AST node
 */
export function d3F(name, declType, anns, valueExpr) {
    return { kind: 'defDec', name, declType, annotations: anns, value: valueExpr };
}

/** Assn: assignment  left := right */
export function a5N(left, right) {
    return { kind: 'assn', left, right };
}

/** IdentifierDeclaration: a single identifier in a parameter list */
export function i0D(name, declType = nil) {
    return { kind: 'identifierDeclaration', name, declType };
}

/** InterpStr: interpolated string  "prefix{expr}post..." */
export function i0S(pre, expr, post) {
    return { kind: 'interpStr', pre: String(pre), expr, post };
}

/**
 * MethodDecl: method declaration
 * parts:      ConsList of Part nodes
 * returnType: AST node or nil
 * anns:       ConsList of annotation nodes
 * body:       ConsList of body statements
 */
export function m0D(parts, returnType, anns, body) {
    return { kind: 'methDec', parts, returnType, annotations: anns, body };
}

/** ReturnStmt: explicit return */
export function r3T(expr) {
    return { kind: 'returnStmt', value: expr };
}

/** Comment node */
export function c0M(text) {
    return { kind: 'comment', text: String(text) };
}

/** ImportStmt: import "name" as asName */
export function i0M(name, asName) {
    // asName may be a string or an identifierDeclaration node
    const asNameStr = (asName && typeof asName === 'object' && asName.kind === 'identifierDeclaration')
        ? asName.name
        : String(asName);
    return { kind: 'importStmt', name: String(name), asName: asNameStr };
}

/**
 * TypeDec: type declaration
 * name:         string
 * genericParams: ConsList of names
 * typeExpr:     AST node
 */
export function t0D(name, genericParams, typeExpr) {
    return { kind: 'typeDec', name: String(name), genericParams, typeExpr };
}

/** InterfaceCons: interface literal */
export function i0C(body) {
    return { kind: 'interfaceCons', body };
}

/** MethSig: method signature (used in interfaces) */
export function m0S(parts, returnType = nil) {
    return { kind: 'methSig', parts, returnType };
}

/** DialectStmt: dialect declaration */
export function dia(name) {
    return { kind: 'dialectStmt', name: String(name) };
}

/** InheritStmt: inherit expression */
export function i0H(parent, extra) {
    return { kind: 'inheritStmt', parent, extra };
}

/** UseStmt: use expression */
export function u0S(parent, extra) {
    return { kind: 'useStmt', parent, extra };
}

/** l0N: lineup literal [ exp1, exp2, ... ] */
export function l0N(elements) {
    return { kind: 'lineup', elements };
}

/** s4F: pre + mid + post string concatenation */
export function s4F(pre, mid, post) {
    // mid is an AST node (strLit or numLit etc.) or a string
    let midStr;
    if (typeof mid === 'string') {
        midStr = mid;
    } else if (mid && mid.kind === 'strLit') {
        midStr = mid.value;
    } else if (mid && mid.kind === 'numLit') {
        midStr = String(mid.value);
    } else {
        midStr = String(mid);
    }
    return String(pre) + midStr + String(post);
}

//  Character escape constants

export const c9D = '$';
export const c9B = '\\';
export const c9Q = '"';
export const c9N = '\n';
export const c9R = '\r';
export const c9L = '{';
export const c9S = '*';
export const c9T = '~';
export const c9G = '`';
export const c9C = '^';
export const c9A = '@';
export const c9P = '%';
export const c9M = '&';
export const c9H = '#';
export const c9E = '!';

// Convert ConsList to JS Array
export function consList(list) {
    const arr = [];
    let cur = list;
    while (cur && cur.kind === 'cons') {
        arr.push(cur.head);
        cur = cur.tail;
    }
    return arr;
}

// Collect all exports for use in eval() context
export const conciseBuilders = {
    nil,
    c0N, o1N, c2N,
    s0L, n0M,
    l0R, d0R, p0T,
    o0C, b1K,
    v4R, d3F, a5N,
    i0D, i0S, m0D,
    r3T, c0M, i0M,
    t0D, i0C, m0S,
    s4F, dia, l0N,
    i0H, u0S,
    c9D, c9B, c9Q, c9N, c9R, c9L, c9S, c9T,
    c9G, c9C, c9A, c9P, c9M, c9H, c9E,
};
