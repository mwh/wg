(* F# & OCaml hybrid *)

type part = | Part of string * astNode list

and astNode =
    | ObjectConstructor of astNode list
    | VarDecl of string * astNode list * string list * astNode list
    | DefDecl of string * astNode list * string list * astNode
    | ExplicitRequest of astNode * part list
    | LexicalRequest of part list
    | NumberNode of double
    | Block of astNode list * astNode list
    | MethodDecl of part list * astNode list * string list * astNode list
    | Assign of astNode * astNode
    | ReturnStmt of astNode
    | IdentifierDeclaration of string * astNode list
    | StringNode of string
    | InterpString of string * astNode * astNode
    | ImportStmt of string * astNode
    | Comment of string



let cons (hd, tl) = hd :: tl
let one hd = [hd]
let nil = []
let no = nil


let objCons (l) = ObjectConstructor l
let varDec (name, dtype, anns, value) = VarDecl (name, dtype, anns, value)
let defDec (name, dtype, anns, value) = DefDecl (name, dtype, anns, value)
let methDec (parts, rtype, anns, body) = MethodDecl (parts, rtype, anns, body)
let dotReq (receiver, req) = ExplicitRequest (receiver, req)
let lexReq parts = LexicalRequest parts
let numLit value = NumberNode value
let strLit value = StringNode value
let interpStr (left, expr, rest) = InterpString (left, expr, rest)
let assn (lhs, rhs) = Assign (lhs, rhs)
let returnStmt value = ReturnStmt value
let identifierDeclaration (name, dtype) = IdentifierDeclaration (name, dtype)
let block (parameters, body) = Block (parameters, body)
let comment text = Comment text
let importStmt (path, binding) = ImportStmt (path, binding)

let part (name, parameters) = Part (name, parameters)

let safeStr (s, a, b) = s ^ a ^ b

let charDollar = "$";
let charBackslash = "\\";
let charDQuote = "\"";
let charLF = "\n";
let charCR = "\r";
let charLBrace = "\{";
let charStar = "*";
let charTilde = "~";
let charBacktick = "`";
let charCaret = "^";
let charAt = "@";
let charPercent = "%";
let charAmp = "&";
let charHash = "#";
let charExclam = "!";

let program = objCons(cons(importStmt("ast", identifierDeclaration("ast", nil)), cons(comment(" This file makes use of all AST nodes"), cons(defDec("x", nil, nil, objCons(cons(varDec("y", one(lexReq(one(part("Number", nil)))), nil, one(numLit(1))), one(methDec(cons(part("foo", one(identifierDeclaration("arg", one(lexReq(one(part("Action", nil))))))), one(part("bar", one(identifierDeclaration("n", nil))))), one(lexReq(one(part("String", nil)))), nil, cons(assn(dotReq(lexReq(one(part("self", nil))), one(part("y", nil))), dotReq(dotReq(lexReq(one(part("arg", nil))), one(part("apply", nil))), one(part("+", one(lexReq(one(part("n", nil)))))))), one(returnStmt(interpStr(safeStr("y ", charAt, " "), lexReq(one(part("y", nil))), strLit(safeStr("", charExclam, ""))))))))), nil)), one(lexReq(one(part("print", one(dotReq(lexReq(one(part("x", nil))), cons(part("foo", one(block(nil, one(numLit(2))))), one(part("bar", one(numLit(3)))))))))))))), nil)
