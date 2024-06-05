module Ast where

data ASTNode = ObjectConstructor [ASTNode]
             | VarDecl String [ASTNode] [String] [ASTNode]
             | DefDecl String [ASTNode] [String] ASTNode
             | ExplicitRequest ASTNode [RequestPart]
             | LexicalRequest [RequestPart]
             | NumberNode Int
             | Block [ASTNode] [ASTNode]
             | MethodDecl [DeclarationPart] [ASTNode] [String] [ASTNode]
             | Assign ASTNode ASTNode
             | ReturnStmt ASTNode
             | IdentifierDeclaration String [ASTNode]
             | StringNode String
             | Comment String
        deriving Show

data RequestPart = RequestPart String [ASTNode]
        deriving Show

data DeclarationPart = DeclarationPart String [ASTNode]
        deriving Show


cons (hd,tl) = hd : tl
nil = []


objectConstructor (l) = ObjectConstructor l
varDecl (name, dtype, anns, val) = VarDecl name dtype anns val
defDecl (name, dtype, anns, val) = DefDecl name dtype anns val
methodDecl (parts, rtype, anns, body) = MethodDecl parts rtype anns body
explicitRequest (receiver, req) = ExplicitRequest receiver req
lexicalRequest = LexicalRequest
numberNode = NumberNode
stringNode = StringNode
assign (lhs, rhs) = Assign lhs rhs
returnStmt = ReturnStmt
identifierDeclaration (name, dtype) = IdentifierDeclaration name dtype
block (params, body) = Block params body
comment = Comment


requestPart (name, args) = RequestPart name args
declarationPart (name, params) = DeclarationPart name params

ppDeclarationPart (DeclarationPart name params) = "declarationPart(\"" ++ name ++ "\", " ++ ppASTList params ++ ")"

ppRequestPart (RequestPart name args) = "requestPart(\"" ++ name ++ "\", " ++ ppASTList args ++ ")"

ppStrList [] = "nil"
ppStrList (h:t) = "cons(\"" ++ h ++ "\", " ++ (ppStrList t) ++ ")"

ppList [] = "nil"
ppList (h:t) = "cons(" ++ h ++ ", " ++ ppList t ++ ")"

ppASTList l = case l of
             [] -> "nil"
             (h:t) -> "cons(" ++ (prettyPrint h) ++ ", " ++ (ppASTList t) ++ ")"

prettyPrint :: ASTNode -> String
prettyPrint (ObjectConstructor body) = "objectConstructor(" ++ (ppASTList body) ++ ")"
prettyPrint (VarDecl name dtype anns val) =
        "varDecl(\"" ++
                name ++ "\", " ++
                (ppASTList dtype) ++ ", " ++
                ppStrList anns ++ ", " ++
                ppASTList val ++ ")"
prettyPrint (DefDecl name dtype anns val) =
        "defDecl(\"" ++
                name ++ "\", " ++
                (ppASTList dtype) ++ ", " ++
                ppStrList anns ++ ", " ++
                prettyPrint val ++ ")"
prettyPrint (MethodDecl parts rtype anns body) =
        "methodDecl(" ++
                ppList (map ppDeclarationPart parts) ++ ", " ++
                ppASTList rtype ++ ", " ++
                ppStrList anns ++ ", " ++
                ppASTList body ++ ")"
prettyPrint (ExplicitRequest receiver req) =
        "explicitRequest(" ++
                prettyPrint receiver ++ ", " ++
                ppList (map ppRequestPart req) ++ ")"
prettyPrint (LexicalRequest req) =
        "lexicalRequest(" ++
                ppList (map ppRequestPart req) ++ ")"
prettyPrint (NumberNode val) = "numberNode(" ++ (show val) ++ ")"
prettyPrint (StringNode val) = "stringNode(\"" ++ val ++ "\")"
prettyPrint (Assign lhs rhs) =
        "assign(" ++ prettyPrint lhs ++ ", " ++ prettyPrint rhs ++ ")"
prettyPrint (ReturnStmt val) = "returnStmt(" ++ prettyPrint val ++ ")"
prettyPrint (IdentifierDeclaration name dtype) =
        "identifierDeclaration(" ++
                "\"" ++ name ++ "\", " ++
                ppASTList dtype ++ ")"
prettyPrint (Block params body) =
        "block(" ++
                ppASTList params ++ ", " ++
                ppASTList body ++ ")"
prettyPrint (Comment text) = "comment(\"" ++ text ++ "\")"