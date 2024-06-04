method EOFToken(index) {
    object {
        def nature is public = "EOF"
        def position is public = index

        method asString {
            nature
        }
    }
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
    object {
        def nature is public = "LPAREN"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method RParenToken(index) {
    object {
        def nature is public = "RPAREN"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method LBraceToken(index) {
    object {
        def nature is public = "LBRACE"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method RBraceToken(index) {
    object {
        def nature is public = "RBRACE"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method CommaToken(index) {
    object {
        def nature is public = "COMMA"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method DotToken(index) {
    object {
        def nature is public = "DOT"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
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

method CommentToken(index, val) {
    object {
        def nature is public = "COMMENT"
        def value is public = val
        def position is public = index

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ position.asString ++ "]"
        }
    }
}

method EqualsToken(index) {
    object {
        def nature is public = "EQUALS"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method AssignToken(index) {
    object {
        def nature is public = "ASSIGN"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method ArrowToken(index) {
    object {
        def nature is public = "ARROW"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method NewlineToken(index) {
    object {
        def nature is public = "NEWLINE"
        def position is public = index

        method asString {
            nature ++ "[" ++ position.asString ++ "]"
        }
    }
}

method isOperatorCharacter(c) {
    (c == "+") || (c == "-") || (c == "*") || (c == "/") || (c == "=") || (c == ":") || (c == "|") || (c == "&") || (c == "!") || (c == ">") || (c == "<") || (c == ".")
}

method isIdentifierStart(c) {
    (c == "a") || (c == "b") || (c == "c") || (c == "d") || (c == "e") || (c == "f") || (c == "g") || (c == "h") || (c == "i") || (c == "j") || (c == "k") || (c == "l") || (c == "m") || (c == "n") || (c == "o") || (c == "p") || (c == "q") || (c == "r") || (c == "s") || (c == "t") || (c == "u") || (c == "v") || (c == "w") || (c == "x") || (c == "y") || (c == "z") || (c == "A") || (c == "B") || (c == "C") || (c == "D") || (c == "E") || (c == "F") || (c == "G") || (c == "H") || (c == "I") || (c == "J") || (c == "K") || (c == "L") || (c == "M") || (c == "N") || (c == "O") || (c == "P") || (c == "Q") || (c == "R") || (c == "S") || (c == "T") || (c == "U") || (c == "V") || (c == "W") || (c == "X") || (c == "Y") || (c == "Z") || (c == "_")
}

method isDigit(c) {
    (c == "1") || (c == "2") || (c == "3") || (c == "4") || (c == "5") || (c == "6") || (c == "7") || (c == "8") || (c == "9") || (c == "0")
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
                return OperatorToken(location, op)
            }

            if (c.firstCP == 13) then {
                c := source.at(index)
                index := index + 1
            }
            if (c.firstCP == 10) then {
                line := line + 1
                lineStart := index - 1
                return NewlineToken(location)
            }

            if (c.firstCP == 34) then {
                var value := ""
                var escaped := false
                while {(source.at(index).firstCP != 34) || escaped} do {
                    var escapeNext := false
                    if ((source.at(index).firstCP == 92) && (escaped == false)) then {
                        escapeNext := true
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

            print("Unknown character: " ++ c.asString ++ " at index " ++ index.asString)
        
        }

        method current {
            currentToken
        }

        method advance {
            currentToken := nextToken
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

"------- Methods from here until the end marker are duplicated in scope in Java"
method cons(hd, tl) {
    object {
        def head is public = hd
        def tail is public = tl
        def end is public = false

        method asString {
            return "cons(" ++ head.asString ++ ", " ++ tail.asString ++ ")"
            return reversed(nil).stringHelper
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
            "nil"
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
            "numberNode(" ++ value.asString ++ ")"
        }
    }
}

method stringNode(val) {
    object {
        def value is public = val

        method asString {
            "stringNode(\"" ++ value ++ "\")"
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

method defDecl(id, anns, val) {
    object {
        def name is public = id
        def annotations is public = anns
        def value is public = val

        method asString {
            "defDecl(\"" ++ name ++ "\", " ++ anns.map { x -> "\"" ++ x ++ "\"" } ++ ", " ++ value ++ ")"
        }
    }
}

method varDecl(id, anns, val) {
    object {
        def name is public = id
        def annotations is public = anns
        def value is public = val

        method asString {
            "varDecl(\"" ++ name ++ "\", " ++ anns.map { x -> "\"" ++ x ++ "\"" } ++ ", " ++ value ++ ")"
        }
    }
}

method lexicalRequest(requestParts) {
    object {
        def parts is public = requestParts

        method asString {
            "lexicalRequest(" ++ parts ++ ")"
        }
    }
}

method explicitRequest(pos, rec, requestParts) {
    object {
        def receiver is public = rec
        def parts is public = requestParts
        def position is public = pos

        method asString {
            "explicitRequest(" ++ receiver.asString ++ ", " ++ parts ++ ")"
        }
    }
}

method requestPart(partName, args) {
    object {
        def name is public = partName
        def arguments is public = args

        method asString {
            "requestPart(\"" ++ name ++ "\", " ++ args ++ ")"
        }
    }
}

method methodDecl(declarationParts, bd) {
    object {
        def parts is public = declarationParts
        def body is public = bd

        method asString {
            "methodDecl(" ++ parts ++ ", " ++ body ++ ")"
        }
    }
}

method declarationPart(id, params) {
    object {
        def name is public = id
        def parameters is public = params

        method asString {
            "declarationPart(\"" ++ name ++ "\", " ++ parameters ++ ")"
        }
    }
}

method objectConstructor(bd) {
    object {
        def body is public = bd

        method asString {
            "objectConstructor(" ++ body ++ ")"
        }
    }

}

method assign(lhs, rhs) {
    object {
        def left is public = lhs
        def right is public = rhs

        method asString {
            "assign(" ++ left ++ ", " ++ right ++ ")"
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

method identifierDeclaration(id) {
    object {
        def name is public = id

        method asString {
            "identifierDeclaration(\"" ++ name ++ "\")"
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

method parselexicalRequest(lxr) {
    def parts = parserequestParts(lxr)
    lexicalRequest(parts)
}

method parserequestParts(lxr) {
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
        } elseif { lxr.current.nature == "LBRACE" } then {
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

method parseExplicitRequest(receiver, lxr) {
    lxr.advance
    def pos = lxr.current.asString
    def parts = parserequestParts(lxr)
    explicitRequest(pos, receiver, parts)
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
    var anns := nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    lxr.advance
    def val = parseExpression(lxr)
    
    defDecl(name, anns, val)
}

method parseVarDeclaration(lxr) {
    lxr.advance
    def name = lxr.current.value
    lxr.advance
    var anns := nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    if (lxr.current.nature == "ASSIGN") then {
        lxr.advance
        def val = parseExpression(lxr)
        varDecl(name, anns, val)
    } else {
        varDecl(name, anns, nil)
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
    def body = parseMethodBody(lxr)
    methodDecl(parts.reversed(nil), body)
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
    methodDecl(parts.reversed(nil), cons(obj, nil))
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