module Program where

import Ast

program = objCons(cons(comment("import \"ast\" as ast"), cons(comment(" This file makes use of all AST nodes"), cons(defDec("x", nil, nil, objCons(cons(varDec("y", one(lexReq(one(part("Number", nil)))), nil, one(numLit(1))), one(methDec(cons(part("foo", one(identifierDeclaration("arg", one(lexReq(one(part("Action", nil))))))), one(part("bar", one(identifierDeclaration("n", nil))))), one(lexReq(one(part("String", nil)))), nil, cons(assn(dotReq(lexReq(one(part("self", nil))), one(part("y", nil))), dotReq(dotReq(lexReq(one(part("arg", nil))), one(part("apply", nil))), one(part("+", one(lexReq(one(part("n", nil)))))))), one(returnStmt(interpStr(safeStr("y ", charAt, " "), lexReq(one(part("y", nil))), strLit(safeStr("", charExclam, ""))))))))), nil)), one(lexReq(one(part("print", one(dotReq(lexReq(one(part("x", nil))), cons(part("foo", one(block(nil, one(numLit(2))))), one(part("bar", one(numLit(3)))))))))))))), nil)
