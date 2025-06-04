module Ast where

data ASTNode = ObjectConstructor [ASTNode] [String]
             | VarDecl String [ASTNode] [String] [ASTNode]
             | DefDecl String [ASTNode] [String] ASTNode
             | ExplicitRequest ASTNode [Part]
             | LexicalRequest [Part]
             | NumberNode Float
             | Block [ASTNode] [ASTNode]
             | MethodDecl [Part] [ASTNode] [String] [ASTNode]
             | Assign ASTNode ASTNode
             | ReturnStmt ASTNode
             | IdentifierDeclaration String [ASTNode]
             | StringNode String
             | InterpString String ASTNode ASTNode
             | Comment String
             | ImportStmt String ASTNode
             | DialectStmt String
        deriving Show

data Part = Part String [ASTNode]
        deriving Show


cons = uncurry (:)
one hd = [hd]
nil = []
no = nil


objCons (l, a) = ObjectConstructor l a
varDec (name, dtype, anns, val) = VarDecl name dtype anns val
defDec (name, dtype, anns, val) = DefDecl name dtype anns val
methDec (parts, rtype, anns, body) = MethodDecl parts rtype anns body
dotReq (receiver, req) = ExplicitRequest receiver req
lexReq = LexicalRequest
numLit = NumberNode
strLit = StringNode
interpStr (before, expr, next) = InterpString before expr next
assn (lhs, rhs) = Assign lhs rhs
returnStmt = ReturnStmt
identifierDeclaration (name, dtype) = IdentifierDeclaration name dtype
block (params, body) = Block params body
comment = Comment
importStmt (name, binding) = ImportStmt name binding
dialectStmt name = DialectStmt name


part (name, params) = Part name params

safeStr (l, m, r) = l ++ m ++ r

charDollar = "$";
charBackslash = "\\";
charDQuote = "\"";
charLF = "\n";
charCR = "\r";
charLBrace = "{";
charStar = "*";
charTilde = "~";
charBacktick = "`";
charCaret = "^";
charAt = "@";
charPercent = "%";
charAmp = "&";
charHash = "#";
charExclam = "!";


ppPart (Part name params) = "part(\"" ++ name ++ "\", " ++ ppASTList params ++ ")"

ppStrList [] = "nil"
ppStrList (h:t) = "cons(\"" ++ h ++ "\", " ++ (ppStrList t) ++ ")"

ppList [] = "nil"
ppList (h:t) = "cons(" ++ h ++ ", " ++ ppList t ++ ")"

ppASTList l = case l of
             [] -> "nil"
             (h:t) -> "cons(" ++ (prettyPrint h) ++ ", " ++ (ppASTList t) ++ ")"

prettyPrint :: ASTNode -> String
prettyPrint (ObjectConstructor body anns) = "objCons(" ++ (ppASTList body) ++ ", " ++(ppStrList anns) ++ ")"
prettyPrint (VarDecl name dtype anns val) =
        "varDec(\"" ++
                name ++ "\", " ++
                (ppASTList dtype) ++ ", " ++
                ppStrList anns ++ ", " ++
                ppASTList val ++ ")"
prettyPrint (DefDecl name dtype anns val) =
        "defDec(\"" ++
                name ++ "\", " ++
                (ppASTList dtype) ++ ", " ++
                ppStrList anns ++ ", " ++
                prettyPrint val ++ ")"
prettyPrint (MethodDecl parts rtype anns body) =
        "methDec(" ++
                ppList (map ppPart parts) ++ ", " ++
                ppASTList rtype ++ ", " ++
                ppStrList anns ++ ", " ++
                ppASTList body ++ ")"
prettyPrint (ExplicitRequest receiver req) =
        "dotReq(" ++
                prettyPrint receiver ++ ", " ++
                ppList (map ppPart req) ++ ")"
prettyPrint (LexicalRequest req) =
        "lexReq(" ++
                ppList (map ppPart req) ++ ")"
prettyPrint (NumberNode val) = "numLit(" ++ (show val) ++ ")"
prettyPrint (StringNode val) = "strLit(\"" ++ val ++ "\")"
prettyPrint (InterpString before expr next) =
        "interpStr(\"" ++ before ++ "\", " ++
                prettyPrint expr ++ ", \"" ++ prettyPrint next ++ "\")"
prettyPrint (Assign lhs rhs) =
        "assn(" ++ prettyPrint lhs ++ ", " ++ prettyPrint rhs ++ ")"
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
prettyPrint (ImportStmt name binding) =
        "importStmt(\"" ++ name ++ "\", " ++ prettyPrint binding ++ ")"
prettyPrint (DialectStmt name) =
        "dialectStmt(\"" ++ name ++ "\")"
