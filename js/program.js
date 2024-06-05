import * as ast from "./ast.js";

function cons(head, tail) {
    return [head, ...tail];
}
const nil = [];

function objectConstructor(body) {
    return new ast.ObjectConstructor(body);
}

function methodDecl(parts, returnType, annotations, body) {
    return new ast.MethodDecl(parts, returnType, annotations, body);
}

function declarationPart(name, params) {
    return new ast.DeclarationPart(name, params);
}

function identifierDeclaration(name, type) {
    return new ast.IdentifierDeclaration(name, type);
}

function lexicalRequest(parts) {
    return new ast.LexicalRequest(parts);
}

function requestPart(name, args) {
    return new ast.RequestPart(name, args);
}

function explicitRequest(receiver, parts) {
    return new ast.ExplicitRequest(receiver, parts);
}

function defDecl(name, type, annotations, value) {
    return new ast.DefDecl(name, type, annotations, value);
}

function varDecl(name, type, annotations, value) {
    return new ast.VarDecl(name, type, annotations, value);
}

function numberNode(value) {
    return new ast.NumberNode(value);
}

function stringNode(value) {
    return new ast.StringNode(value);
}

function block(params, body) {
    return new ast.Block(params, body);
}

function comment(value) {
    return new ast.Comment(value);
}


export const program = objectConstructor(cons(
    methodDecl(cons(declarationPart("succ",
        cons(identifierDeclaration("x", cons(lexicalRequest(cons(requestPart("Number", nil), nil)), nil)), nil)), nil),
        cons(lexicalRequest(cons(requestPart("Number", nil), nil)), nil), 
        cons("public", nil),
        cons(explicitRequest(lexicalRequest(cons(requestPart("x", nil), nil)), cons(requestPart("+", cons(numberNode(1), nil)), nil)), nil)),
    cons(defDecl("a", nil, nil, numberNode(1)),
    cons(varDecl("b", nil, nil, cons(lexicalRequest(cons(requestPart("succ", cons(lexicalRequest(cons(requestPart("a", nil), nil)), nil)), nil)), nil)),
    cons(explicitRequest(block(nil,
        cons(lexicalRequest(cons(requestPart("print", cons(lexicalRequest(cons(requestPart("b", nil), nil)), nil)), nil)), nil)),
        cons(requestPart("apply", nil), nil)), nil)))))