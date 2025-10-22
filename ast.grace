method cons(hd, tl) {
    object {
        def head is public = hd
        def tail is public = tl
        def end is public = false
        def kind is public = "cons"

        method asString {
            if (tail.end) then {
                return "one(" ++ head.asString ++ ")"
            }
            return "cons(" ++ head.asString ++ ", " ++ tail.asString ++ ")"
        }

        method reversed(next) {
            def c = cons(head, next)
            if (tail.end) then {
                c
            } else {
                tail.reversed(c)
            }
        }

        method map(f) {
            cons(f.apply(head), tail.map(f))
        }
    }
}

method nil {
    object {
        def end is public = true
        def kind is public = "nil"

        method asString {
            "nil"
        }

        method reversed(next) {
            next
        }

        method map(f) {
            nil
        }
    }

}

method numberNode(val) {
    object {
        def value is public = val
        def kind is public = "numLit"

        method asString {
            "numLit(" ++ value.asString ++ ")"
        }
    }
}

method stringNode(val) {
    object {
        def value is public = val
        def kind is public = "strLit"

        method asString {
            "strLit(" ++ escapeString(value) ++ ")"
        }
    }
}

method interpString(val, exp, rest) {
    object {
        def value is public = val
        def expression is public = exp
        def next is public = rest
        def kind is public = "interpStr"

        method asString {
            "interpStr(" ++ escapeString(value) ++ ", " ++ expression ++ ", " ++ next ++ ")"
        }
    }
}

method block(params, stmts) {
    object {
        def parameters is public = params
        def statements is public = stmts
        def kind is public = "block"

        method asString {
            "block(" ++ parameters ++ ", " ++ statements ++ ")"
        }
    }
}

method defDecl(id, dtype, anns, val) {
    object {
        def name is public = id
        def decType is public = dtype
        def annotations is public = anns
        def value is public = val
        def kind is public = "defDec"

        method asString {
            "defDec(\"" ++ name ++ "\", "  ++ decType ++ ", " ++ anns.map { x -> "\"" ++ x ++ "\"" } ++ ", " ++ value ++ ")"
        }
    }
}


method typeDecl(id, val) {
    object {
        def name is public = id
        def value is public = val
        def kind is public = "typeDec"

        method asString {
            "typeDec(\"" ++ name ++ "\", " ++ value ++ ")"
        }
    }
}

method interfaceCons(bd) {
    object {
        def body is public = bd
        def kind is public = "interfaceCons"

        method asString {
            "interfaceCons(" ++ body ++ ")"
        }
    }
}

method methSig(pts, rType) {
    object {
        def parts is public = pts
        def returnType is public = rType
        def kind is public = "methSig"

        method asString {
            "methSig(" ++ parts ++ ", " ++ returnType ++ ")"
        }
    }
}

method varDecl(id, dtype, anns, val) {
    object {
        def name is public = id
        def decType is public = dtype
        def annotations is public = anns
        def value is public = val
        def kind is public = "varDec"

        method asString {
            "varDec(\"" ++ name ++ "\", " ++ dtype ++ ", " ++ anns.map { x -> "\"" ++ x ++ "\"" } ++ ", " ++ value ++ ")"
        }
    }
}

method lexicalRequest(requestParts) {
    object {
        def parts is public = requestParts
        def kind is public = "lexReq"

        method asString {
            "lexReq(" ++ parts ++ ")"
        }
    }
}

method lexicalRequest(pos, requestParts) {
    object {
        def parts is public = requestParts
        def position is public = pos
        def kind is public = "lexReq"

        method asString {
            "lexReq(" ++ parts ++ ")"
        }
    }
}

method explicitRequest(pos, rec, requestParts) {
    object {
        def receiver is public = rec
        def parts is public = requestParts
        def position is public = pos
        def kind is public = "dotReq"

        method asString {
            "dotReq(" ++ receiver.asString ++ ", " ++ parts ++ ")"
        }
    }
}

method requestPart(partName, args) {
    object {
        def name is public = partName
        def arguments is public = args

        method asString {
            "part(\"" ++ name ++ "\", " ++ args ++ ")"
        }
    }
}


method part(partName, args) {
    object {
        def name is public = partName
        def parameters is public = args
        def kind is public = "part"

        method asString {
            "part(\"" ++ name ++ "\", " ++ parameters ++ ")"
        }
    }
}

method methodDecl(declarationParts, retType, anns, bd) {
    object {
        def parts is public = declarationParts
        def returnType is public = retType
        def annotations is public = anns
        def body is public = bd
        def kind is public = "methDec"

        method asString {
            "methDec(" ++ parts ++ ", " ++ returnType ++ ", " ++ annotations ++ ", " ++ body ++ ")"
        }
    }
}

method declarationPart(id, params) {
    object {
        def name is public = id
        def parameters is public = params

        method asString {
            "part(\"" ++ name ++ "\", " ++ parameters ++ ")"
        }
    }
}

method objectConstructor(bd, anns) {
    object {
        def body is public = bd
        def annotations is public = anns
        def kind is public = "objCons"

        method asString {
            "objCons(" ++ body ++ ", " ++ annotations.map { x -> "\"" ++ x ++ "\"" } ++ ")"
        }
    }

}

method assign(lhs, rhs) {
    object {
        def left is public = lhs
        def right is public = rhs
        def kind is public = "assn"

        method asString {
            "assn(" ++ left ++ ", " ++ right ++ ")"
        }
    }
}

method returnStmt(val) {
    object {
        def value is public = val
        def kind is public = "returnStmt"

        method asString {
            "returnStmt(" ++ value ++ ")"
        }
    }
}

method identifierDeclaration(id, dtype) {
    object {
        def name is public = id
        def decType is public = dtype
        def kind is public = "identifierDeclaration"

        method asString {
            "identifierDeclaration(" ++ escapeString(name) ++ ", " ++ dtype ++ ")"
        }
    }
}

method comment(text) {
    object {
        def value is public = text
        def kind is public = "comment"

        method asString {
            "comment(" ++ escapeString(text) ++ ")"
        }
    }
}

method importStmt(src, nm) {
    object {
        def source is public = src
        def binding is public = nm
        def kind is public = "importStmt"

        method asString {
            "importStmt(\"" ++ source ++ "\", " ++ binding ++ ")"
        }
    }
}

method dialectStmt(src) {
    object {
        def source is public = src
        def kind is public = "dialectStmt"

        method asString {
            "dialectStmt(\"" ++ source ++ "\")"
        }
    }
}

method escapeString(value) {
    var i := 1
    def len = value.size
    while { i <= len } do {
        def c = value.at(i)
        if (c == "\\") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charBackslash, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "$") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charDollar, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "*") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charStar, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\{") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charLBrace, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\n") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charLF, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\r") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charCR, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\"") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charDQuote, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "~") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charTilde, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "^") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charCaret, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "`") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charBacktick, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "@") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charAt, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "&") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charAmp, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "%") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charPercent, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "#") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charHash, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "!") then {
            return "safeStr(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", charExclam, " ++ escapeString(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        i := i + 1
    }
    "\"" ++ value.replace "\\" with "\\\\".replace "\"" with "\\\"".replace "\n" with "\\n".replace "\r" with "\\r" ++ "\""
}
