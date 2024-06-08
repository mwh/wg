method cons(hd, tl) {
    object {
        def head is public = hd
        def tail is public = tl
        def end is public = false

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

        method asString {
            "no"
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

        method asString {
            "numLit(" ++ value.asString ++ ")"
        }
    }
}

method stringNode(val) {
    object {
        def value is public = val

        method asString {
            "strLit(\"" ++ escapeString(value) ++ "\")"
        }
    }
}

method block(params, stmts) {
    object {
        def parameters is public = params
        def statements is public = stmts

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

        method asString {
            "defDec(\"" ++ name ++ "\", "  ++ decType ++ ", " ++ anns.map { x -> "\"" ++ x ++ "\"" } ++ ", " ++ value ++ ")"
        }
    }
}

method varDecl(id, dtype, anns, val) {
    object {
        def name is public = id
        def decType is public = dtype
        def annotations is public = anns
        def value is public = val

        method asString {
            "varDec(\"" ++ name ++ "\", " ++ dtype ++ ", " ++ anns.map { x -> "\"" ++ x ++ "\"" } ++ ", " ++ value ++ ")"
        }
    }
}

method lexicalRequest(requestParts) {
    object {
        def parts is public = requestParts

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

method objectConstructor(bd) {
    object {
        def body is public = bd

        method asString {
            "objCons(" ++ body ++ ")"
        }
    }

}

method assign(lhs, rhs) {
    object {
        def left is public = lhs
        def right is public = rhs

        method asString {
            "assn(" ++ left ++ ", " ++ right ++ ")"
        }
    }
}

method returnStmt(val) {
    object {
        def value is public = val

        method asString {
            "returnStmt(" ++ value ++ ")"
        }
    }
}

method identifierDeclaration(id, dtype) {
    object {
        def name is public = id
        def decType is public = dtype

        method asString {
            "identifierDeclaration(\"" ++ name ++ "\", " ++ dtype ++ ")"
        }
    }
}

method comment(text) {
    object {
        def value is public = text

        method asString {
            "comment(\"" ++ escapeString(text) ++ "\")"
        }
    }
}

method importStmt(src, nm) {
    object {
        def source is public = src
        def binding is public = nm

        method asString {
            "importStmt(\"" ++ source ++ "\", " ++ binding ++ ")"
        }
    }
}

method escapeString(value) {
    value.replace "\\" with "\\\\".replace "\"" with "\\\"".replace "\n" with "\\n".replace "\r" with "\\r"
}
