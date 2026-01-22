method cons(hd, tl) {
    object {
        def head is public = hd
        def tail is public = tl
        def end is public = false
        def kind is public = "cons"
        def size = tl.size + 1

        method asString {
            if (tail.end) then {
                return "one(" ++ head.asString ++ ")"
            }
            return "cons(" ++ head.asString ++ ", " ++ tail.asString ++ ")"
        }

        method concise {
            if (tail.end) then {
                return "o1N(" ++ head.concise ++ ")"
            }
            if (size == 2) then {
                return "c2N(" ++ head.concise ++ "," ++ tail.head.concise ++ ")"
            }
            return "c0N(" ++ head.concise ++ "," ++ tail.concise ++ ")"
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

        method concise {
            "nil"
        }

        method size {
            0
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

        method concise {
            "n0M(" ++ value.asString ++ ")"
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

        method concise {
            "s0L(" ++ escapeStringConcise(value) ++ ")"
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
            "interpStr(" ++ escapeString(value) ++ "," ++ expression ++ ", " ++ next ++ ")"
        }

        method concise {
            "i0S(" ++ escapeStringConcise(value) ++ "," ++ expression.concise ++ "," ++ next.concise ++ ")"
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

        method concise {
            "b1K(" ++ parameters.concise ++ "," ++ statements.concise ++ ")"
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

        method concise {
            "d3F(\"" ++ name ++ "\","  ++ decType.concise ++ "," ++ (anns.map { x -> "\"" ++ x ++ "\"" }.concise) ++ "," ++ value.concise ++ ")"
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

        method concise {
            "t0D(\"" ++ name ++ "\"," ++ value.concise ++ ")"
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

        method concise {
            "i0C(" ++ body.concise ++ ")"
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

        method concise {
            "m0S(" ++ parts.concise ++ "," ++ returnType.concise ++ ")"
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

        method concise {
            "v4R(\"" ++ name ++ "\"," ++ dtype.concise ++ "," ++ (anns.map { x -> "\"" ++ x ++ "\"" }.concise) ++ "," ++ value.concise ++ ")"
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

        method concise {
            var name := ""
            var args := nil
            parts.map { p -> 
                name := name ++ p.name
                name := name ++ "(" ++ p.parameters.size.asString ++ ")"
                p.parameters.map { a -> 
                    args := cons(a, args)
                    true
                }
            }
            "l0R(\"" ++ name ++ "\"," ++ args.reversed(nil).concise ++ ")"
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

        method concise {
            var name := ""
            var args := nil
            parts.map { p -> 
                name := name ++ p.name
                name := name ++ "(" ++ p.parameters.size.asString ++ ")"
                p.parameters.map { a -> 
                    args := cons(a, args)
                    true
                }
            }
            "l0R(\"" ++ name ++ "\"," ++ args.reversed(nil).concise ++ ")"
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

        method concise {
            var name := ""
            var args := nil
            parts.map { p -> 
                name := name ++ p.name
                name := name ++ "(" ++ p.parameters.size.asString ++ ")"
                p.parameters.map { a -> 
                    args := cons(a, args)
                    true
                }
            }
            "d0R(" ++ receiver.concise ++ ",\"" ++ name ++ "\"," ++ args.reversed(nil).concise ++ ")"
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

        method concise {
            "p0T(\"" ++ name ++ "\"," ++ parameters.concise ++ ")"
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

        method concise {
            "m0D(" ++ parts.concise ++ "," ++ returnType.concise ++ "," ++ (annotations.map { x -> "\"" ++ x ++ "\"" }.concise) ++ "," ++ body.concise ++ ")"
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

        method concise {
            "o0C(" ++ body.concise ++ "," ++ (annotations.map { x -> "\"" ++ x ++ "\"" }.concise) ++ ")"
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

        method concise {
            "a5N(" ++ left.concise ++ "," ++ right.concise ++ ")"
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

        method concise {
            "r3T(" ++ value.concise ++ ")"
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

        method concise {
            "i0D(" ++ escapeStringConcise(name) ++ "," ++ dtype.concise ++ ")"
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

        method concise {
            "c0M(" ++ escapeStringConcise(text) ++ ")"
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

        method concise {
            "i0M(\"" ++ source ++ "\"," ++ binding.concise ++ ")"
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

        method concise {
            "d0S(\"" ++ source ++ "\")"
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

method escapeStringConcise(value) {
    var i := 1
    def len = value.size
    while { i <= len } do {
        def c = value.at(i)
        if (c == "\\") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", c9B, " ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "$") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\", c9D, " ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "*") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9S," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\{") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9L," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\n") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9N," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\r") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9R," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "\"") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9Q," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "~") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9T," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "^") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9C," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "`") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9G," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "@") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9A," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "&") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9M," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "%") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9P," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "#") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9H," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        if (c == "!") then {
            return "s4F(\"" ++ value.substringFrom 1 to(i - 1) ++ "\",c9E," ++ escapeStringConcise(value.substringFrom(i + 1)to(len)) ++ ")"
        }
        i := i + 1
    }
    "\"" ++ value.replace "\\" with "\\\\".replace "\"" with "\\\"".replace "\n" with "\\n".replace "\r" with "\\r" ++ "\""
}
