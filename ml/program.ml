(* F# & OCaml hybrid *)

type requestPart = | RequestPart of string * astNode list

and declarationPart = | DeclarationPart of string * astNode list

and astNode =
    | ObjectConstructor of astNode list
    | VarDecl of string * astNode list * string list * astNode list
    | DefDecl of string * astNode list * string list * astNode
    | ExplicitRequest of astNode * requestPart list
    | LexicalRequest of requestPart list
    | NumberNode of int
    | Block of astNode list * astNode list
    | MethodDecl of declarationPart list * astNode list * string list * astNode list
    | Assign of astNode * astNode
    | ReturnStmt of astNode
    | IdentifierDeclaration of string * astNode list
    | StringNode of string
    | Comment of string



let cons (hd, tl) = hd :: tl
let one hd = [hd]
let nil = []


let objectConstructor (l) = ObjectConstructor l
let varDecl (name, dtype, anns, value) = VarDecl (name, dtype, anns, value)
let defDecl (name, dtype, anns, value) = DefDecl (name, dtype, anns, value)
let methodDecl (parts, rtype, anns, body) = MethodDecl (parts, rtype, anns, body)
let explicitRequest (receiver, req) = ExplicitRequest (receiver, req)
let lexicalRequest parts = LexicalRequest parts
let numberNode value = NumberNode value
let stringNode value = StringNode value
let assign (lhs, rhs) = Assign (lhs, rhs)
let returnStmt value = ReturnStmt value
let identifierDeclaration (name, dtype) = IdentifierDeclaration (name, dtype)
let block (parameters, body) = Block (parameters, body)
let comment text = Comment text

let requestPart (name, args) = RequestPart (name, args)
let declarationPart (name, parameters) = DeclarationPart (name, parameters)

let program = objectConstructor(cons(methodDecl(cons(declarationPart("succ", cons(identifierDeclaration("x", cons(lexicalRequest(cons(requestPart("Number", nil), nil)), nil)), nil)), nil), cons(lexicalRequest(cons(requestPart("Number", nil), nil)), nil), cons("public", nil), cons(explicitRequest(lexicalRequest(cons(requestPart("x", nil), nil)), cons(requestPart("+", cons(numberNode(1), nil)), nil)), nil)), cons(defDecl("a", nil, nil, numberNode(1)), cons(varDecl("b", nil, nil, cons(lexicalRequest(cons(requestPart("succ", cons(lexicalRequest(cons(requestPart("a", nil), nil)), nil)), nil)), nil)), cons(explicitRequest(block(nil, cons(lexicalRequest(cons(requestPart("print", cons(lexicalRequest(cons(requestPart("b", nil), nil)), nil)), nil)), nil)), cons(requestPart("apply", nil), nil)), nil)))))
