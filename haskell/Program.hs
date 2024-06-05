module Program where

import Ast

program = objectConstructor(cons(methodDecl(cons(declarationPart("succ", cons(identifierDeclaration("x", cons(lexicalRequest(cons(requestPart("Number", nil), nil)), nil)), nil)), nil), cons(lexicalRequest(cons(requestPart("Number", nil), nil)), nil), cons("public", nil), cons(explicitRequest(lexicalRequest(cons(requestPart("x", nil), nil)), cons(requestPart("+", cons(numberNode(1), nil)), nil)), nil)), cons(defDecl("a", nil, nil, numberNode(1)), cons(varDecl("b", nil, nil, cons(lexicalRequest(cons(requestPart("succ", cons(lexicalRequest(cons(requestPart("a", nil), nil)), nil)), nil)), nil)), cons(explicitRequest(block(nil, cons(lexicalRequest(cons(requestPart("print", cons(lexicalRequest(cons(requestPart("b", nil), nil)), nil)), nil)), nil)), cons(requestPart("apply", nil), nil)), nil)))))
