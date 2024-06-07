method EOFToken(index) {
    SymbolToken(index, "EOF")
}

method NumberToken(index, val) {
    object {
        def nature is public = "NUMBER"
        def value is public = val
        def position is public = index

        method asString {
            nature ++ "(" ++ value ++ ")[" ++ position.asString ++ "]"
        }
    }
}

method LParenToken(index) {
    SymbolToken(index, "LPAREN")
}

method RParenToken(index) {
    SymbolToken(index, "RPAREN")
}

method LBraceToken(index) {
    SymbolToken(index, "LBRACE")
}

method RBraceToken(index) {
    SymbolToken(index, "RBRACE")
}

method CommaToken(index) {
    SymbolToken(index, "COMMA")
}

method DotToken(index) {
    SymbolToken(index, "DOT")
}

method IdentifierToken(index, val) {
    object {
        def nature is public = "IDENTIFIER"
        def value is public = val
        def position is public = index

        method asString {
            nature ++ "(" ++ value ++ ")[" ++ position.asString ++ "]"
        }
    }
}

method KeywordToken(index, val) {
    object {
        def nature is public = "KEYWORD"
        def value is public = val
        def position is public = index

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ position.asString ++ "]"
        }
    }
}

method OperatorToken(index, val) {
    object {
        def nature is public = "OPERATOR"
        def value is public = val
        def position is public = index

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ position.asString ++ "]"
        }
    }
}

method StringToken(index, val) {
    object {
        def nature is public = "STRING"
        def value is public = val
        def position is public = index

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ position.asString ++ "]"
        }
    }
}

method EqualsToken(index) {
    SymbolToken(index, "EQUALS")
}

method AssignToken(index) {
    SymbolToken(index, "ASSIGN")
}

method ArrowToken(index) {
    SymbolToken(index, "ARROW")
}

method SymbolToken(index, nat) {
    object {
        def nature is public = nat
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method NewlineToken(index) {
    SymbolToken(index, "NEWLINE")
}

method CommentToken(index, text) {
    object {
        def nature is public = "COMMENT"
        def position is public = index
        def value is public = text

        method asString {
            nature ++ "(" ++ value ++ ")[" ++ position.asString ++ "]"
        }
    }
}

method isOperatorCharacter(c) {
    (c == "+") || (c == "-") || (c == "*") || (c == "/") || (c == "=") || (c == ":") || (c == "|") || (c == "&") || (c == "!") || (c == ">") || (c == "<") || (c == ".")
}

method isIdentifierStart(c) {
    def cp = c.firstCodepoint
    ((cp >= 97) && (cp <= 122)) || ((cp >= 65) && (cp <= 90)) || (cp == 95)
}

method isDigit(c) {
    def cp = c.firstCodepoint
    (cp >= 48) && (cp <= 57)
}

method lexer(code) {
    object {
        def source = code
        var index := 1
        var line := 1
        var column := 0
        var lineStart := 0
        var currentToken := nextToken

        method nextToken {
            if (index > source.size) then {
                return EOFToken(index)
            }

            var c := source.at(index)
            column := index - lineStart
            index := index + 1

            def location = line.asString ++ ":" ++ column.asString
            

            if (c == " ") then {
                return nextToken
            }

            if (c == "(") then {
                return LParenToken(location)
            }

            if (c == ")") then {
                return RParenToken(location)
            }

            if ((c > "z") && (c < "|")) then {
                return LBraceToken(location)
            }

            if (c == "}") then {
                return RBraceToken(location)
            }

            if (c == ",") then {
                return CommaToken(location)
            }

            if (c.firstCodepoint == 13) then {
                c := source.at(index)
                index := index + 1
            }
            if ((c.firstCodepoint == 10) || (c.firstCodepoint == 8232)) then {
                line := line + 1
                lineStart := index - 1
                return NewlineToken(location)
            }

            if (c.firstCodepoint == 34) then {
                var value := ""
                var escaped := false
                while {(source.at(index).firstCodepoint != 34) || escaped} do {
                    var escapeNext := false
                    def cp = source.at(index).firstCodepoint
                    if ((cp == 92) && (escaped == false)) then {
                        escapeNext := true
                    } elseif { escaped && (cp == 110) } then {
                        value := value ++ "\n"
                    } elseif { escaped && (cp == 114) } then {
                        value := value ++ "\r"
                    } else {
                        value := value ++ source.at(index)
                    }
                    escaped := escapeNext
                    index := index + 1
                }
                index := index + 1
                return StringToken(location, value)
            }

            if (isDigit(c)) then {
                def startIndex = index - 1
                if (index >= source.size) then {
                    return NumberToken(location, c)
                }
                var value := ""
                while {isDigit(c) && (index <= source.size)} do {
                    value := value ++ c
                    c := source.at(index)
                    index := index + 1
                }
                if (index > (startIndex + 1)) then {
                    index := index - 1
                }
                return NumberToken(location, value)
            }

            if (isIdentifierStart(c)) then {
                def startIndex = index - 1
                var value := c
                if (index > source.size) then {
                    return IdentifierToken(location, value)
                }
                c := source.at(index)
                while {(isIdentifierStart(c) || isDigit(c) || (c == "'")) && (index <= source.size)} do {
                    value := value ++ c
                    index := index + 1
                    if (index <= source.size) then {
                        c := source.at(index)
                    }
                }
                if ((value == "var") || (value == "def") || (value == "method") || (value == "object") || (value == "is") || (value == "return") || (value == "class") || (value == "type")) then {
                    return KeywordToken(location, value)
                }
                return IdentifierToken(location, value)
            }

            if (isOperatorCharacter(c)) then {
                def startIndex = index
                var op := c
                c := source.at(index)
                index := index + 1
                while {isOperatorCharacter(c) && (index <= source.size)} do {
                    op := op ++ c
                    c := source.at(index)
                    index := index + 1
                }
                index := index - 1
                if (op == ":=") then {
                    return AssignToken(location)
                }
                if (op == "=") then {
                    return EqualsToken(location)
                }
                if (op == "->") then {
                    return ArrowToken(location)
                }
                if (op == ".") then {
                    return DotToken(location)
                }
                if (op == ":") then {
                    return SymbolToken(location, "COLON")
                }
                if (op == "//") then {
                    var cp := c.firstCodepoint
                    var text := ""
                    index := index + 1
                    while { (cp != 10) && (cp != 13) && (index <= source.size) } do {
                        text := text ++ c
                        c := source.at(index)
                        cp := c.firstCodepoint
                        index := index + 1
                    }
                    return CommentToken(location, text)
                }
                return OperatorToken(location, op)
            }

            print("Unknown character: " ++ c.asString ++ "(" ++ c.firstCodepoint.asString ++ ") at index " ++ index.asString)
        
        }

        method current {
            currentToken
        }

        method advance {
            currentToken := nextToken
        }

        method expectToken(nature) {
            if (currentToken.nature != nature) then {
                print("Expected " ++ nature ++ " but got " ++ currentToken.nature ++ " at " ++ currentToken.position.asString)
                Exception.raise("Expected " ++ nature ++ " but got " ++ currentToken.nature ++ " at " ++ currentToken.position.asString)
            }
        }

        method expectSymbol(nature) {
            if (currentToken.nature != nature) then {
                print("Expected " ++ nature ++ " but got " ++ currentToken.nature ++ " at " ++ currentToken.position.asString)
                Exception.raise("Expected " ++ nature ++ " but got " ++ currentToken.nature ++ " at " ++ currentToken.position.asString)
            }
        }

        method expectKeyword(val) {
            if (currentToken.nature != "KEYWORD") then {
                print("Expected KEYWORD but got " ++ currentToken.nature ++ " at " ++ currentToken.position.asString)
                Exception.raise("Expected KEYWORD but got " ++ currentToken.nature ++ " at " ++ currentToken.position.asString)
            }
            if (currentToken.value != val) then {
                print("Expected " ++ val ++ " but got " ++ currentToken.value ++ " at " ++ currentToken.position.asString)
                Exception.raise("Expected " ++ val ++ " but got " ++ currentToken.value ++ " at " ++ currentToken.position.asString)
            }
        }
    }
}

method digitToNumber(c) {
    if (c == "1") then {
        return 1
    }
    if (c == "2") then {
        return 2
    }
    if (c == "3") then {
        return 3
    }
    if (c == "4") then {
        return 4
    }
    if (c == "5") then {
        return 5
    }
    if (c == "6") then {
        return 6
    }
    if (c == "7") then {
        return 7
    }
    if (c == "8") then {
        return 8
    }
    if (c == "9") then {
        return 9
    }
    if (c == "0") then {
        return 0
    }
    print("Unexpected digit: " ++ c.asString)
    Exception.raise("Unexpected digit: " ++ c.asString)
}

method escapeString(value) {
    value.replace "\\" with "\\\\".replace "\"" with "\\\"".replace "\n" with "\\n".replace "\r" with "\\r"
}

"------- Methods from here until the end marker are duplicated in scope in Java"
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

        method stringHelper {
            "cons(" ++ head.asString ++ ", " ++ tail.stringHelper ++ ")"
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

        method stringHelper {
            "nil"
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

"------- End marker; methods back to the start marker are duplicated in scope in Java"

method parseNumber(lxr) {
    def token = lxr.current
    lxr.advance
    def s = token.value
    var val := 0
    var index := 1
    while { index <= s.size } do {
        val := val * 10 + digitToNumber(s.at(index))
        index := index + 1
    }
    numberNode(val)
}

method parseString(lxr) {
    def token = lxr.current
    lxr.advance
    stringNode(token.value)
}

method parseLexicalRequestNoBlock(lxr, id) {
    def parts = parseRequestParts(lxr, false)
    lexicalRequest(parts)
}

method parselexicalRequest(lxr) {
    def parts = parseRequestParts(lxr, true)
    lexicalRequest(parts)
}

method parseRequestParts(lxr, allowBlock) {
    var parts := nil
    while {lxr.current.nature == "IDENTIFIER"} do {
        var id := lxr.current.value
        lxr.advance
        if (lxr.current.nature == "LPAREN") then {
            lxr.advance
            var args := nil
            while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                args := cons(parseExpression(lxr), args)
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            lxr.advance
            def part = requestPart(id, args.reversed(nil))
            parts := cons(part, parts)
        } elseif { allowBlock && (lxr.current.nature == "LBRACE") } then {
            def blk = parseBlock(lxr)
            def part = requestPart(id, cons(blk, nil))
            parts := cons(part, parts)
        } elseif { lxr.current.nature == "NUMBER" } then {
            def num = parseNumber(lxr)
            def part = requestPart(id, cons(num, nil))
            parts := cons(part, parts)
        } elseif { lxr.current.nature == "STRING" } then {
            def str = parseString(lxr)
            def part = requestPart(id, cons(str, nil))
            parts := cons(part, parts)
        } else {
            def part = requestPart(id, nil)
            parts := cons(part, parts)
            return parts.reversed(nil)
        }
    }
    parts.reversed(nil)
}

method parseExplicitRequestNoBlock(receiver, lxr) {
    lxr.advance
    def pos = lxr.current.asString
    def parts = parseRequestParts(lxr, false)
    explicitRequest(pos, receiver, parts)
}

method parseExplicitRequest(receiver, lxr) {
    lxr.advance
    def pos = lxr.current.asString
    def parts = parseRequestParts(lxr, true)
    explicitRequest(pos, receiver, parts)
}

method parseTypeExpression(lxr) {
    def token = lxr.current
    if (token.nature == "IDENTIFIER") then {
        return parseLexicalRequestNoBlock(lxr, token.value)
    }
    print("Unexpected token: " ++ token.asString)
    Exception.raise("Unexpected token: " ++ token.asString)
}

method parseExpressionNoOpNoDot(lxr) {
    def token = lxr.current
    if (token.nature == "NUMBER") then {
        return parseNumber(lxr)
    }
    if (token.nature == "STRING") then {
        return parseString(lxr)
    }
    if (token.nature == "LPAREN") then {
        lxr.advance
        def expr = parseExpression(lxr)
        lxr.advance
        return expr
    }
    if (token.nature == "LBRACE") then {
        return parseBlock(lxr)
    }
    if (token.nature == "IDENTIFIER") then {
        return parselexicalRequest(lxr)
    }
    if (token.nature == "KEYWORD") then {
        if (token.value == "object") then {
            return parseObject(lxr)
        }
    }
    if (token.nature == "OPERATOR") then {
        lxr.advance
        def pos = lxr.current.asString
        def expr = parseExpressionNoOp(lxr)
        return explicitRequest(pos, expr, cons(requestPart("prefix" ++ token.value, nil), nil))
    }
    print("Unexpected token: " ++ token.asString)
    Exception.raise("Unexpected token: " ++ token.asString)
}

method parseExpressionNoOp(lxr) {
    var left := parseExpressionNoOpNoDot(lxr)
    var token := lxr.current
    while { token.nature == "DOT" } do {
        left := parseExplicitRequest(left, lxr)
        
        token := lxr.current
    }
    return left
}

method parseExpression(lxr) {
    var left := parseExpressionNoOp(lxr)
    var token := lxr.current
    while { token.nature == "OPERATOR" } do {
        def pos = token.asString
        lxr.advance
        def right = parseExpressionNoOp(lxr)
        def args = cons(right, nil)
        def part = requestPart(token.value, args)
        def parts = cons(part, nil)
        def req = explicitRequest(pos, left, parts.reversed(nil))
        left := req
        token := lxr.current
    }
    return left
}

method parseReturnStatement(lxr) {
    lxr.advance
    def val = parseExpression(lxr)
    returnStmt(val)
}

method parseStatement(lxr) {
    def token = lxr.current
    if (token.nature == "KEYWORD") then {
        if (token.value == "return") then {
            return parseReturnStatement(lxr)
        }
        if (token.value == "var") then {
            return parseVarDeclaration(lxr)
        }
        if (token.value == "def") then {
            return parseDefDeclaration(lxr)
        }
    } elseif {token.nature == "COMMENT"} then {
        lxr.advance
        return comment(token.value)
    }
    var exp := parseExpression(lxr)
    if (lxr.current.nature == "ASSIGN") then {
        lxr.advance
        def val = parseExpression(lxr)
        exp := assign(exp, val)
    }
    if (lxr.current.nature == "NEWLINE") then {
        lxr.advance
    }
    exp
}

method parseBlock(lxr) {
    var params := nil
    var body := nil
    if (lxr.current.nature == "LBRACE") then {
        lxr.advance
        while { lxr.current.nature == "NEWLINE" } do {
            lxr.advance
        }
        while {lxr.current.nature != "RBRACE"} do {
            if (lxr.current.nature == "ARROW") then {
                lxr.advance
                params := body
                body := nil
            } else {
                body := cons(parseStatement(lxr), body)
            }
            while { lxr.current.nature == "NEWLINE" } do {
                lxr.advance
            }
        }
        lxr.advance
    }

    block(params, body.reversed(nil))

}

method parseAnnotations(lxr) {
    var anns := nil
    lxr.advance
    while {lxr.current.nature == "IDENTIFIER"} do {
        anns := cons(lxr.current.value, anns)
        lxr.advance
        if (lxr.current.nature == "COMMA") then {
            lxr.advance
        }
    }
    anns.reversed(nil)
}

method parseDefDeclaration(lxr) {
    lxr.advance
    def name = lxr.current.value
    lxr.advance
    var dtype := nil
    if (lxr.current.nature == "COLON") then {
        // Type annotation
        lxr.advance
        dtype := cons(parseExpression(lxr), nil)
    }
    var anns := nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    lxr.advance
    def val = parseExpression(lxr)
    
    defDecl(name, dtype, anns, val)
}

method parseVarDeclaration(lxr) {
    lxr.advance
    def name = lxr.current.value
    lxr.advance
    var dtype := nil
    if (lxr.current.nature == "COLON") then {
        // Type annotation
        lxr.advance
        dtype := cons(parseExpression(lxr), nil)
    }
    var anns := nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    if (lxr.current.nature == "ASSIGN") then {
        lxr.advance
        def val = parseExpression(lxr)
        varDecl(name, dtype, anns, cons(val, nil))
    } else {
        varDecl(name, dtype, anns, nil)
    }
}

method parseMethodBody(lxr) {
    var body := nil
    lxr.advance
    while {lxr.current.nature != "RBRACE"} do {
        if (lxr.current.nature == "NEWLINE") then {
            lxr.advance
        } elseif { lxr.current.nature == "KEYWORD" } then {
            if (lxr.current.value == "var") then {
                def dec = parseVarDeclaration(lxr)
                body := cons(dec, body)
            } elseif { lxr.current.value == "def" } then {
                def dec = parseDefDeclaration(lxr)
                body := cons(dec, body)
            } else {
                body := cons(parseStatement(lxr), body)
            }
        } else {
            body := cons(parseStatement(lxr), body)
        }
    }
    lxr.advance
    body.reversed(nil)

}

method parseMethodDeclaration(lxr) {
    lxr.advance
    var parts := nil
    while { lxr.current.nature == "IDENTIFIER" } do {
        def id = lxr.current.value
        lxr.advance
        if (lxr.current.nature == "LPAREN") then {
            lxr.advance
            var args := nil
            while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                lxr.expectToken "IDENTIFIER"
                def idToken = lxr.current
                var dtype := nil
                lxr.advance
                if (lxr.current.nature == "COLON") then {
                    // Type annotation
                    lxr.advance
                    dtype := cons(parseTypeExpression(lxr), nil)
                }
                args := cons(identifierDeclaration(idToken.value, dtype), args)
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            lxr.advance
            def part = declarationPart(id, args.reversed(nil))
            parts := cons(part, parts)
        } else {
            def part = declarationPart(id, nil)
            parts := cons(part, parts)
        }
    }
    var dtype := nil
    if (lxr.current.nature == "ARROW") then {
        // Type annotation
        lxr.advance
        dtype := cons(parseTypeExpression(lxr), nil)
    }
    var anns := nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    //lxr.advance
    def body = parseMethodBody(lxr)
    methodDecl(parts.reversed(nil), dtype, anns, body)
}

method parseClassDeclaration(lxr) {
    lxr.advance
    var parts := nil
    while { lxr.current.nature == "IDENTIFIER" } do {
        def id = lxr.current.value
        lxr.advance
        if (lxr.current.nature == "LPAREN") then {
            lxr.advance
            var args := nil
            while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                args := cons(identifierDeclaration(lxr.current.value), args)
                lxr.advance
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            lxr.advance
            def part = declarationPart(id, args.reversed(nil))
            parts := cons(part, parts)
        } else {
            def part = declarationPart(id, nil)
            parts := cons(part, parts)
        }
    }
    lxr.advance
    def body = parseObjectBody(lxr)
    def obj = objectConstructor(body)
    methodDecl(parts.reversed(nil), nil, nil, cons(obj, nil))
}

method parseObjectBody(lxr) {
    var body := nil
    
    var token := lxr.current

    while { (token.nature != "EOF") && (token.nature != "RBRACE") } do {
        if (token.nature == "NEWLINE") then {
            lxr.advance
        } elseif { token.nature == "KEYWORD" } then {
            if (token.value == "var") then {
                def dec = parseVarDeclaration(lxr)
                body := cons(dec, body)
            } elseif { token.value == "def" } then {
                def dec = parseDefDeclaration(lxr)
                body := cons(dec, body)
            } elseif { token.value == "method" } then {
                def dec = parseMethodDeclaration(lxr)
                body := cons(dec, body)
            } elseif { token.value == "class" } then {
                def dec = parseClassDeclaration(lxr)
                body := cons(dec, body)
            } else {
                def stmt = parseStatement(lxr)
                body := cons(stmt, body)
            } 
        } else {
            def stmt = parseStatement(lxr)
            body := cons(stmt, body)
        }
        token := lxr.current
    }
    
    body.reversed(nil)

}

method parseObject(lxr) {
    lxr.advance
    lxr.advance
    def body = parseObjectBody(lxr)
    lxr.advance
    objectConstructor(body)
}

method parse(code) {
    def lxr = lexer(code)
    def body = parseObjectBody(lxr)
    objectConstructor(body)
}

def str = getFileContents "wg.grace"
def pr = parse(str)
print(pr)