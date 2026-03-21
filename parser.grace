import "ast" as ast
import "lexer" as lexer

method parseNumber(lxr) {
    def token = lxr.current
    lxr.advance
    def s = token.value
    var val := 0
    var index := 1
    while { (index <= s.size) } do {
        if (s.at(index) == ".") then {
            index := index + 1
            var frac := 0
            var scale := 1
            while {index <= s.size} do {
                frac := (frac * 10) + lexer.digitToNumber(token, s.at(index))
                scale := scale * 10
                index := index + 1
            }
            val := val + (frac / scale)
        } else {
            val := (val * 10) + lexer.digitToNumber(token, s.at(index))
        }
        index := index + 1
    }
    ast.numberNode(val)
}

method parseString(lxr) {
    def token = lxr.current
    lxr.advance
    if (token.nature == "INTERPSTRING") then {
        def interpExpr = parseExpression(lxr)
        lxr.expectSymbol("RBRACE")
        lxr.startStringAt(lxr.current.index)
        def nextStr = parseString(lxr)
        ast.interpString(token.value, interpExpr, nextStr)
    } else {
        ast.stringNode(token.value)
    }
}

method parselexicalRequestNoBlock(lxr, id) {
    def pos = lxr.current.location
    def parts = parseparts(lxr, false)
    ast.lexicalRequest(pos, parts)
}

method parselexicalRequest(lxr) {
    def pos = lxr.current.location
    def parts = parseparts(lxr, true)
    ast.lexicalRequest(pos, parts)
}

method parseparts(lxr, allowBlock) {
    var parts := ast.nil
    while {lxr.current.nature == "IDENTIFIER"} do {
        var id := lxr.current.value
        var genericParams := ast.nil
        lxr.advance
        if (lxr.current.nature == "LGENERIC") then {
            lxr.advance
            while { (lxr.current.nature != "RGENERIC") && (lxr.current.nature != "EOF") } do {
                genericParams := ast.cons(parseTypeExpression(lxr), genericParams)
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            if (lxr.current.nature != "RGENERIC") then {
                parseError(lxr.current.line, lxr.current.column, "Expected ']]' to close generic argument list")
            }
            lxr.advance
            genericParams := genericParams.reversed(ast.nil)
        }
        if (lxr.current.nature == "LPAREN") then {
            lxr.advance
            var args := ast.nil
            while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                args := ast.cons(parseExpression(lxr), args)
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            lxr.advance
            def part = ast.part(id, args.reversed(ast.nil), genericParams)
            parts := ast.cons(part, parts)
        } elseif { allowBlock && (lxr.current.nature == "LBRACE") } then {
            def blk = parseblock(lxr)
            def part = ast.part(id, ast.cons(blk, ast.nil), genericParams)
            parts := ast.cons(part, parts)
        } elseif { lxr.current.nature == "NUMBER" } then {
            def num = parseNumber(lxr)
            def part = ast.part(id, ast.cons(num, ast.nil), genericParams)
            parts := ast.cons(part, parts)
        } elseif { (lxr.current.nature == "STRING") || (lxr.current.nature == "INTERPSTRING") } then {
            def str = parseString(lxr)
            def part = ast.part(id, ast.cons(str, ast.nil), genericParams)
            parts := ast.cons(part, parts)
        } else {
            if (lxr.current.nature == "LSQUARE") then {
                def lineup = parseLineup(lxr)
                def part = ast.part(id, ast.cons(lineup, ast.nil), genericParams)
                parts := ast.cons(part, parts)
            } else {
                def part = ast.part(id, ast.nil, genericParams)
                parts := ast.cons(part, parts)
                return parts.reversed(ast.nil)
            }
        }
    }
    parts.reversed(ast.nil)
}

method parseexplicitRequestNoBlock(receiver, lxr) {
    lxr.advance
    def pos = lxr.current.location
    def parts = parseparts(lxr, false)
    ast.explicitRequest(pos, receiver, parts)
}

method parseexplicitRequest(receiver, lxr) {
    lxr.advance
    def pos = lxr.current.location
    def parts = parseparts(lxr, true)
    ast.explicitRequest(pos, receiver, parts)
}

method parseInterface(lxr) {
    lxr.expectKeyword "interface"
    lxr.advance
    lxr.expectSymbol "LBRACE"
    lxr.advance

    def indentBefore = lexer.indentColumn
    lexer.indentColumn := lxr.current.column
    var body := ast.nil
    while { (lxr.current.nature == "IDENTIFIER") || (lxr.current.nature == "OPERATOR") } do {
        var parts := ast.nil
        var first := true
        while { (lxr.current.nature == "IDENTIFIER") || (first && (lxr.current.nature == "OPERATOR")) } do {
            first := false
            def id = lxr.current.value
            lxr.advance
            var genericParams := ast.nil
            if (lxr.current.nature == "LGENERIC") then {
                lxr.advance
                while { (lxr.current.nature != "RGENERIC") && (lxr.current.nature != "EOF") } do {
                    lxr.expectToken "IDENTIFIER"
                    def idToken = lxr.current
                    lxr.advance
                    genericParams := ast.cons(ast.identifierDeclaration(idToken.value, ast.nil), genericParams)
                    if (lxr.current.nature == "COMMA") then {
                        lxr.advance
                    }
                }
                if (lxr.current.nature != "RGENERIC") then {
                    parseError(lxr.current.line, lxr.current.column, "Expected close of generic parameter list, but got " ++ lxr.current.asString )
                }
                lxr.advance
                genericParams := genericParams.reversed(ast.nil)
            }
            if (lxr.current.nature == "LPAREN") then {
                lxr.advance
                var args := ast.nil
                while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                    lxr.expectToken "IDENTIFIER"
                    def idToken = lxr.current
                    var dtype := ast.nil
                    lxr.advance
                    if (lxr.current.nature == "COLON") then {
                        // Type annotation
                        lxr.advance
                        dtype := ast.cons(parseTypeExpression(lxr), ast.nil)
                    }
                    args := ast.cons(ast.identifierDeclaration(idToken.value, dtype), args)
                    if (lxr.current.nature == "COMMA") then {
                        lxr.advance
                    }
                }
                lxr.advance
                def part = ast.part(id, args.reversed(ast.nil), genericParams)
                parts := ast.cons(part, parts)
            } else {
                def part = ast.part(id, ast.nil, genericParams)
                parts := ast.cons(part, parts)
            }
        }
        var dtype := ast.nil
        if (lxr.current.nature == "ARROW") then {
            // Type annotation
            lxr.advance
            dtype := ast.cons(parseTypeExpression(lxr), ast.nil)
        }
        body := ast.cons(ast.methSig(parts.reversed(ast.nil), dtype), body)
        while { lxr.current.nature == "NEWLINE" } do {
            lxr.advance
        }
    }

    lxr.expectSymbol "RBRACE"
    lxr.advance
    lexer.indentColumn := indentBefore
    ast.interfaceCons(body.reversed(ast.nil))
}

method parseTypeTerm(lxr) {
    def token = lxr.current
    if (token.nature == "IDENTIFIER") then {
        var ret := parselexicalRequestNoBlock(lxr, token.value)
        if (lxr.current.nature == "DOT") then {
            var token := lxr.current
            while { token.nature == "DOT" } do {
                ret := parseexplicitRequest(ret, lxr)
                token := lxr.current
            }
        }
        return ret
    }
    if (token.nature == "KEYWORD") then {
        if (token.value == "interface") then {
            return parseInterface(lxr)
        }
    }
    if (token.nature == "LBRACE") then {
        parseError(token.line, token.column, "Unexpected token: " ++ token.asString ++ "; did you omit 'interface' before the '\{'?")
    }
    parseError(token.line, token.column, "Unexpected token: " ++ token.asString)
}

method parseTypeExpression(lxr) {
    var expr := parseTypeTerm(lxr)
    if (lxr.current.nature == "OPERATOR") then {
        def op = lxr.current
        lxr.advance
        def term2 = parseTypeExpression(lxr)
        expr := ast.explicitRequest(op.location, expr, ast.cons(ast.part(op.value, ast.cons(term2, ast.nil)), ast.nil))
    }
    expr
}

method parseLineup(lxr) {
    lxr.expectSymbol "LSQUARE"
    lxr.advance
    var elements := ast.nil
    while { (lxr.current.nature != "RSQUARE") && (lxr.current.nature != "EOF") } do {
        def elem = parseExpression(lxr)
        elements := ast.cons(elem, elements)
        if (lxr.current.nature == "COMMA") then {
            lxr.advance
        } elseif { lxr.current.nature != "RSQUARE" } then {
            parseError(lxr.current.line, lxr.current.column, "Expected ',' or ']' in lineup, but got " ++ lxr.current.asString)
        }
    }
    if (lxr.current.nature != "RSQUARE") then {
        parseError(lxr.current.line, lxr.current.column, "Expected ']' to close lineup")
    }
    lxr.advance
    ast.lineup(elements.reversed(ast.nil))
}

method parseTypeExpressionOrPattern(lxr) {
    def token = lxr.current
    if (token.nature == "IDENTIFIER") then {
        return parseTypeExpression(lxr)
    }
    if (token.nature == "NUMBER") then {
        return parseExpression(lxr)
    }
    if (token.nature == "STRING") then {
        return parseExpression(lxr)
    }
    print("Unexpected token: " ++ token.asString)
    parseError(token.line, token.column, "Unexpected token: " ++ token.asString)
}

method parseExpressionNoOpNoDot(lxr) {
    def token = lxr.current
    if (token.nature == "NUMBER") then {
        return parseNumber(lxr)
    }
    if ((token.nature == "STRING") || (lxr.current.nature == "INTERPSTRING")) then {
        return parseString(lxr)
    }
    if (token.nature == "LPAREN") then {
        lxr.advance
        def expr = parseExpression(lxr)
        lxr.skipWhitespace
        lxr.expectSymbol "RPAREN" explaining "Parenthesised group starting at {token} expected to end here."
        lxr.advance
        return expr
    }
    if (token.nature == "LBRACE") then {
        return parseblock(lxr)
    }
    if (token.nature == "LSQUARE") then {
        return parseLineup(lxr)
    }
    if (token.nature == "IDENTIFIER") then {
        return parselexicalRequest(lxr)
    }
    if (token.nature == "KEYWORD") then {
        if (token.value == "object") then {
            return parseObject(lxr)
        } elseif { token.value == "self" } then {
            lxr.advance
            return ast.lexicalRequest(token.location, ast.cons(ast.part("self", ast.nil), ast.nil))
        }
    }
    if (token.nature == "OPERATOR") then {
        lxr.advance
        def pos = token.location
        def expr = parseExpressionNoOp(lxr)
        return ast.explicitRequest(pos, expr, ast.cons(ast.part("prefix" ++ token.value, ast.nil), ast.nil))
    }
    parseError(token.line, token.column, "Unexpected token: " ++ token.asString)
}

method parseExpressionNoOp(lxr) {
    var left := parseExpressionNoOpNoDot(lxr)
    var token := lxr.current
    while { token.nature == "DOT" } do {
        left := parseexplicitRequest(left, lxr)
        
        token := lxr.current
    }
    return left
}

method parseExpression(lxr) {
    var left := parseExpressionNoOp(lxr)
    var token := lxr.current
    if (token.nature == "OPERATOR") then {
        var theOperator := token.value
        while { token.nature == "OPERATOR" } do {
            if (token.value != theOperator) then {
                parseError(token.line, token.column, "Multiple operators mixed without parentheses: found " ++ token.value ++ "; expected only further " ++ theOperator ++ ". Use parentheses to group distinct operators.")
            }
            def pos = token.location
            lxr.advance
            def right = parseExpressionNoOp(lxr)
            def args = ast.cons(right, ast.nil)
            def part = ast.part(token.value, args)
            def parts = ast.cons(part, ast.nil)
            def req = ast.explicitRequest(pos, left, parts.reversed(ast.nil))
            left := req
            token := lxr.current
        }
    }
    return left
}

method parseReturnStatement(lxr) {
    lxr.advance
    def val = parseExpression(lxr)
    endStatement(lxr)
    ast.returnStmt(val)
}

method endStatement(lxr) {
    def nature = lxr.current.nature
    if ((nature != "NEWLINE") && (nature != "SEMICOLON") && (nature != "EOF") && (nature != "RBRACE") && (nature != "COMMENT")) then {
        parseError(lxr.current.line, lxr.current.column, "Expected end of statement, but got " ++ lxr.current.asString)
    }
    if (nature == "SEMICOLON") then {
        lxr.advance
        lxr.expectSymbol "NEWLINE" or "COMMENT" explaining "Semicolon must be at end of line."
    }
}

method parseStatement(lxr) {
    def token = lxr.current
    if (token.nature == "KEYWORD") then {
        if (token.value == "return") then {
            return parseReturnStatement(lxr)
        }
        if (token.value == "var") then {
            return parsevarDeclaration(lxr)
        }
        if (token.value == "def") then {
            return parsedefDeclaration(lxr)
        }
        if (token.value == "type") then {
            return parseTypeDeclaration(lxr)
        }
    } elseif {token.nature == "COMMENT"} then {
        lxr.advance
        if (lxr.current.nature == "NEWLINE") then {
            lxr.advance
        }
        return ast.comment(token.value)
    }
    var exp := parseExpression(lxr)
    if (lxr.current.nature == "ASSIGN") then {
        lxr.advance
        def val = parseExpression(lxr)
        exp := ast.assign(exp, val)
    }
    endStatement(lxr)
    if (lxr.current.nature == "NEWLINE") then {
        lxr.advance
    }
    exp
}

method parseParamOrStatement(lxr) {
    if (lxr.current.nature == "IDENTIFIER") then {
        def memo = lxr.save
        def ident = lxr.current
        lxr.advance
        def nt = lxr.current
        if ((nt.nature == "ARROW") || (nt.nature == "COMMA")) then {
            return ast.identifierDeclaration(ident.value, ast.nil)
        }
        if (nt.nature == "COLON") then {
            lxr.advance
            def tp = parseTypeExpressionOrPattern(lxr)
            return ast.identifierDeclaration(ident.value, ast.cons(tp, ast.nil))
        }
        lxr.restore(memo)
        parseStatement(lxr)
    } elseif { (lxr.current.nature == "STRING") || (lxr.current.nature == "NUMBER") } then {
        // Might be a pattern
        def memo = lxr.save
        def expr = parseExpression(lxr)
        if (lxr.current.nature == "ARROW") then {
            // Definitely a pattern
            return ast.identifierDeclaration("_", ast.cons(expr, ast.nil))
        }
        lxr.restore(memo)
        parseStatement(lxr)
    } else {
        parseStatement(lxr)
    }
}

method parseblock(lxr) {
    var params := ast.nil
    var body := ast.nil
    def indentBefore = lexer.indentColumn
    if (lxr.current.nature == "LBRACE") then {
        lxr.advance
        while { lxr.current.nature == "NEWLINE" } do {
            lxr.advance
        }
        if (lxr.current.nature != "RBRACE") then {
            def firstTok = lxr.current
            if (firstTok.column <= indentBefore) then {
                parseError(firstTok.line, firstTok.column, "Indentation must increase inside block body. Expected at least column " ++ (indentBefore + 1) ++ " on line " ++ firstTok.line ++ " but got " ++ firstTok.column)
            }
            lexer.indentColumn := firstTok.column
            def first = parseParamOrStatement(lxr)
            def after = lxr.current
            if ((after.nature == "ARROW") || (after.nature == "COMMA")) then {
                // Parameter list
                var prm := first
                var tp := ast.nil
                params := ast.cons(prm, params)
                while { (lxr.current.nature != "ARROW") && (lxr.current.nature != "RBRACE") } do {
                    lxr.advance
                    if (lxr.current.nature == "NEWLINE") then {
                        lxr.advance
                    } else {
                        lxr.expectToken "IDENTIFIER"
                        def ident = lxr.current
                        lxr.advance
                        tp := ast.nil
                        if (lxr.current.nature == "COLON") then {
                            lxr.advance
                            tp := parseTypeExpression(lxr)
                        }
                        prm := ast.identifierDeclaration(ident.value, tp)
                        params := ast.cons(prm, params)
                    }
                }
                lxr.expectToken("ARROW")
                lxr.advance
                while { lxr.current.nature == "NEWLINE" } do {
                    lxr.advance
                }
            } else {
                lexer.indentColumn := indentBefore
                body := ast.cons(first, body)
            }
        }
        if (lxr.current.nature == "SEMICOLON") then {
            lxr.advance
            lxr.expectToken("NEWLINE")
        }
        while { lxr.current.nature == "NEWLINE" } do {
            lxr.advance
        }
        while {lxr.current.nature != "RBRACE"} do {
            lexer.indentColumn := lxr.current.column
            if (lexer.indentColumn <= indentBefore) then {
                parseError(lxr.current.line, lexer.indentColumn, "Indentation must increase inside block body. Expected at least column " ++ (indentBefore + 1) ++ " on line " ++ lxr.current.line ++ " but got " ++ lexer.indentColumn)
            }
            body := ast.cons(parseStatement(lxr), body)
            if (lxr.current.nature == "SEMICOLON") then {
                lxr.advance
                lxr.expectToken("NEWLINE")
            }
            while { lxr.current.nature == "NEWLINE" } do {
                lxr.advance
            }
        }
        lexer.indentColumn := indentBefore
        lxr.advance
    }
    ast.block(params.reversed(ast.nil), body.reversed(ast.nil))

}

method parseAnnotations(lxr) {
    var anns := ast.nil
    lxr.advance
    while {lxr.current.nature == "IDENTIFIER"} do {
        anns := ast.cons(lxr.current.value, anns)
        lxr.advance
        if (lxr.current.nature == "COMMA") then {
            lxr.advance
        }
    }
    anns.reversed(ast.nil)
}

method parseTypeDeclaration(lxr) {
    lxr.expectKeyword "type"
    lxr.advance
    lxr.expectToken "IDENTIFIER"
    def ident = lxr.current
    lxr.advance
    var genericParams := ast.nil
    if (lxr.current.nature == "LGENERIC") then {
        lxr.advance
        while { (lxr.current.nature != "RGENERIC") && (lxr.current.nature != "EOF") } do {
            lxr.expectToken "IDENTIFIER"
            def idToken = lxr.current
            lxr.advance
            genericParams := ast.cons(ast.identifierDeclaration(idToken.value, ast.nil), genericParams)
            if (lxr.current.nature == "COMMA") then {
                lxr.advance
            }
        }
        if (lxr.current.nature != "RGENERIC") then {
            parseError(lxr.current.line, lxr.current.column, "Expected close of generic parameter list, but got " ++ lxr.current.asString )
        }
        lxr.advance
        genericParams := genericParams.reversed(ast.nil)
    }
    lxr.expectSymbol "EQUALS"
    lxr.advance
    def typeExpr = parseTypeExpression(lxr)
    endStatement(lxr)
    ast.typeDecl(ident.value, genericParams, typeExpr)
}

method parsedefDeclaration(lxr) {
    lxr.advance
    lxr.expectToken("IDENTIFIER")
    def name = lxr.current.value
    lxr.advance
    var dtype := ast.nil
    if (lxr.current.nature == "COLON") then {
        // Type annotation
        lxr.advance
        dtype := ast.cons(parseExpression(lxr), ast.nil)
    }
    var anns := ast.nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    lxr.expectSymbol "EQUALS" explaining "def declarations must have an initial value assigned with '='."
    lxr.advance
    def val = parseExpression(lxr)
    endStatement(lxr)
    ast.defDecl(name, dtype, anns, val)
}

method parsevarDeclaration(lxr) {
    lxr.advance
    lxr.expectToken("IDENTIFIER")
    def name = lxr.current.value
    lxr.advance
    var dtype := ast.nil
    if (lxr.current.nature == "COLON") then {
        // Type annotation
        lxr.advance
        dtype := ast.cons(parseExpression(lxr), ast.nil)
    }
    var anns := ast.nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    if (lxr.current.nature == "ASSIGN") then {
        lxr.advance
        def val = parseExpression(lxr)
        endStatement(lxr)
        ast.varDecl(name, dtype, anns, ast.cons(val, ast.nil))
    } else {
        endStatement(lxr)
        ast.varDecl(name, dtype, anns, ast.nil)
    }
}

method parseMethodBody(lxr) {
    var body := ast.nil
    lxr.advance
    def indentBefore = lexer.indentColumn
    while {lxr.current.nature != "RBRACE"} do {
        if (lxr.current.nature == "SEMICOLON") then {
            lxr.advance
            lxr.expectToken("NEWLINE")
        }
        if (lxr.current.nature == "NEWLINE") then {
            lxr.advance
        } else {
            lexer.indentColumn := lxr.current.column
            if (lexer.indentColumn <= indentBefore) then {
                parseError(lxr.current.line, lexer.indentColumn, "Indentation must increase inside method body. Expected at least column " ++ (indentBefore + 1) ++ " on line " ++ lxr.current.line ++ " but got " ++ lexer.indentColumn)
            }
            if (lxr.current.nature == "KEYWORD") then {
                if (lxr.current.value == "var") then {
                    def dec = parsevarDeclaration(lxr)
                    body := ast.cons(dec, body)
                } elseif { lxr.current.value == "def" } then {
                    def dec = parsedefDeclaration(lxr)
                    body := ast.cons(dec, body)
                } else {
                    body := ast.cons(parseStatement(lxr), body)
                }
            } else {
                body := ast.cons(parseStatement(lxr), body)
            }
        }
    }
    lxr.advance
    lexer.indentColumn := indentBefore
    body.reversed(ast.nil)

}

method parseMethodDeclaration(lxr) {
    lxr.advance
    var parts := ast.nil
    if (lxr.current.nature == "IDENTIFIER") then {
        while { lxr.current.nature == "IDENTIFIER" } do {
            var genericParams := ast.nil
            var id := lxr.current.value
            lxr.advance
            if ((id == "prefix") && (lxr.current.nature == "OPERATOR")) then {
                id := id ++ lxr.current.value
                lxr.advance
            }
            if (lxr.current.nature == "ASSIGN") then {
                id := id ++ ":="
                lxr.advance
            }
            if (lxr.current.nature == "LGENERIC") then {
                lxr.advance
                while { (lxr.current.nature != "RGENERIC") && (lxr.current.nature != "EOF") } do {
                    lxr.expectToken "IDENTIFIER"
                    def idToken = lxr.current
                    lxr.advance
                    genericParams := ast.cons(ast.identifierDeclaration(idToken.value, ast.nil), genericParams)
                    if (lxr.current.nature == "COMMA") then {
                        lxr.advance
                    }
                }
                if (lxr.current.nature != "RGENERIC") then {
                    parseError(lxr.current.line, lxr.current.column, "Expected close of generic parameter list, but got " ++ lxr.current.asString )
                }
                lxr.advance
                genericParams := genericParams.reversed(ast.nil)
            }
            if (lxr.current.nature == "LPAREN") then {
                lxr.advance
                var args := ast.nil
                while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                    lxr.expectToken "IDENTIFIER"
                    def idToken = lxr.current
                    var dtype := ast.nil
                    lxr.advance
                    if (lxr.current.nature == "COLON") then {
                        // Type annotation
                        lxr.advance
                        dtype := ast.cons(parseTypeExpression(lxr), ast.nil)
                    }
                    args := ast.cons(ast.identifierDeclaration(idToken.value, dtype), args)
                    if (lxr.current.nature == "COMMA") then {
                        lxr.advance
                    }
                }
                lxr.advance
                def part = ast.part(id, args.reversed(ast.nil), genericParams)
                parts := ast.cons(part, parts)
            } else {
                def part = ast.part(id, ast.nil, genericParams)
                parts := ast.cons(part, parts)
            }
        }
    } elseif { lxr.current.nature == "OPERATOR" } then {
        def id = lxr.current.value
        lxr.advance
        lxr.expectSymbol "LPAREN"
        lxr.advance
        var args := ast.nil
        lxr.expectToken "IDENTIFIER"
        def idToken = lxr.current
        var dtype := ast.nil
        lxr.advance
        if (lxr.current.nature == "COLON") then {
            // Type annotation
            lxr.advance
            dtype := ast.cons(parseTypeExpression(lxr), ast.nil)
        }
        args := ast.cons(ast.identifierDeclaration(idToken.value, dtype), args)
        lxr.advance
        def part = ast.part(id, args.reversed(ast.nil), ast.nil)
        parts := ast.cons(part, parts)
    } else {
        parseError(lxr.current.line, lxr.current.column, "Expected method name or operator")
    }
    var dtype := ast.nil
    if (lxr.current.nature == "ARROW") then {
        // Type annotation
        lxr.advance
        dtype := ast.cons(parseTypeExpression(lxr), ast.nil)
    }
    var anns := ast.nil
    if ((lxr.current.nature == "KEYWORD")) then {
        if (lxr.current.value == "is") then {
            anns := parseAnnotations(lxr)
        }
    }
    //lxr.advance
    def body = parseMethodBody(lxr)
    ast.methodDecl(parts.reversed(ast.nil), dtype, anns, body)
}

method parseClassDeclaration(lxr) {
    lxr.advance
    var parts := ast.nil
    while { lxr.current.nature == "IDENTIFIER" } do {
        def id = lxr.current.value
        lxr.advance
        var genericParams := ast.nil
        if (lxr.current.nature == "LGENERIC") then {
            lxr.advance
            while { (lxr.current.nature != "RGENERIC") && (lxr.current.nature != "EOF") } do {
                lxr.expectToken "IDENTIFIER"
                def idToken = lxr.current
                lxr.advance
                genericParams := ast.cons(ast.identifierDeclaration(idToken.value, ast.nil), genericParams)
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            if (lxr.current.nature != "RGENERIC") then {
                parseError(lxr.current.line, lxr.current.column, "Expected close of generic parameter list, but got " ++ lxr.current.asString )
            }
            lxr.advance
            genericParams := genericParams.reversed(ast.nil)
        }
        if (lxr.current.nature == "LPAREN") then {
            lxr.advance
            var args := ast.nil
            while {(lxr.current.nature != "RPAREN") && (lxr.current.nature != "EOF")} do {
                def name = lxr.current.value
                lxr.advance
                var dtype := ast.nil
                if (lxr.current.nature == "COLON") then {
                    // Type annotation
                    lxr.advance
                    dtype := ast.cons(parseTypeExpression(lxr), ast.nil)
                }
                args := ast.cons(ast.identifierDeclaration(name, dtype), args)
                if (lxr.current.nature == "COMMA") then {
                    lxr.advance
                }
            }
            lxr.advance
            def part = ast.part(id, args.reversed(ast.nil), genericParams)
            parts := ast.cons(part, parts)
        } else {
            def part = ast.part(id, ast.nil, genericParams)
            parts := ast.cons(part, parts)
        }
    }
    def start = lxr.current.location
    lxr.advance
    def body = parseObjectBody(lxr)
    lxr.expectSymbol("RBRACE") explaining("Class body starting at " ++ start ++ " did not close properly.")
    lxr.advance
    def obj = ast.objectConstructor(body, ast.nil)
    ast.methodDecl(parts.reversed(ast.nil), ast.nil, ast.nil, ast.cons(obj, ast.nil))
}

method parseImport(lxr) {
    lxr.expectKeyword("import")
    lxr.advance
    lxr.expectToken("STRING")
    def src = lxr.current.value
    lxr.advance
    lxr.expectToken("IDENTIFIER")
    if (lxr.current.value != "as") then {
        parseError(lxr.current.line, lxr.current.column, "Expected 'as' in import")
    }
    lxr.advance
    lxr.expectToken("IDENTIFIER")
    def name = lxr.current.value
    lxr.advance
    var ident
    if (lxr.current.nature == "COLON") then {
        lxr.advance
        def aType = parseTypeExpression(lxr)
        ident := ast.identifierDeclaration(name, aType)
    } else {
        ident := ast.identifierDeclaration(name, ast.nil)
    }
    ast.importStmt(src, ident)
}

method parseDialect(lxr) {
    lxr.expectKeyword("dialect")
    lxr.advance
    lxr.expectToken("STRING")
    def src = lxr.current.value
    lxr.advance
    ast.dialectStmt(src)
}

method parseInherit(lxr) {
    lxr.expectKeyword("inherit")
    lxr.advance
    def expr = parseExpression(lxr)
    ast.inheritStmt(expr, ast.nil)
}

method parseUse(lxr) {
    lxr.expectKeyword("use")
    lxr.advance
    def expr = parseExpression(lxr)
    ast.useStmt(expr, ast.nil)
}

method parseObjectBody(lxr) {
    var body := ast.nil
    
    var token := lxr.current

    def indentBefore = lexer.indentColumn

    var first := true

    while { (token.nature != "EOF") && (token.nature != "RBRACE") } do {
        lexer.indentColumn := token.column
        if ((lexer.indentColumn <= indentBefore) && (token.nature != "NEWLINE")) then {
            parseError(lxr.current.line, lexer.indentColumn, "Indentation must increase inside object body. Expected at least column " ++ (indentBefore + 1) ++ " on line " ++ lxr.current.line ++ " but got " ++ lexer.indentColumn)
        }

        if (token.nature == "SEMICOLON") then {
            lxr.advance
            token := lxr.current
            lxr.expectToken("NEWLINE")
        }
        if (token.nature == "NEWLINE") then {
            lxr.advance
        } elseif { token.nature == "KEYWORD" } then {
            if (token.value == "var") then {
                def dec = parsevarDeclaration(lxr)
                body := ast.cons(dec, body)
            } elseif { token.value == "def" } then {
                def dec = parsedefDeclaration(lxr)
                body := ast.cons(dec, body)
            } elseif { token.value == "method" } then {
                def dec = parseMethodDeclaration(lxr)
                body := ast.cons(dec, body)
            } elseif { token.value == "class" } then {
                def dec = parseClassDeclaration(lxr)
                body := ast.cons(dec, body)
            } else {
                if (token.value == "import") then {
                    def imp = parseImport(lxr)
                    body := ast.cons(imp, body)
                } elseif { token.value == "dialect" } then {
                    if (first) then {
                        def dia = parseDialect(lxr)
                        body := ast.cons(dia, body)
                    } else {
                        parseError(lxr.current.line, lexer.indentColumn, "dialect declaration can only appear as first statement")
                    }
                } elseif { token.value == "inherit" } then {
                    def inh = parseInherit(lxr)
                    body := ast.cons(inh, body)
                } elseif { token.value == "use" } then {
                    def useSt = parseUse(lxr)
                    body := ast.cons(useSt, body)
                } else {
                    def stmt = parseStatement(lxr)
                    body := ast.cons(stmt, body)
                }
            }
            if (first) then {
                if (lxr.current.nature != "COMMENT") then {
                    first := false
                }
            }
        } else {
            if (first) then {
                if (lxr.current.nature != "COMMENT") then {
                    first := false
                }
            }
            def stmt = parseStatement(lxr)
            body := ast.cons(stmt, body)
        }
        token := lxr.current
    }
    
    lexer.indentColumn := indentBefore
    body.reversed(ast.nil)

}

method parseObject(lxr) {
    lxr.advance
    var anns := ast.nil
    if (lxr.current.nature == "KEYWORD") then {
        lxr.expectKeyword("is")
        anns := parseAnnotations(lxr)
    }
    def start = lxr.current
    lxr.advance
    def body = parseObjectBody(lxr)
    lxr.expectSymbol("RBRACE") explaining("Object body starting at " ++ start ++ " did not close properly.")
    lxr.advance
    ast.objectConstructor(body, anns)
}

method parseError(line, column, message) {
    print("Parse error: " ++ message ++ " at " ++ lexer.modulePrefix ++ line ++ ":" ++ column)
    Exception.refine "ParseError".raise(lexer.modulePrefix ++ line ++ ":" ++ column ++ ": " ++ message)
}

method parse(code) {
    lexer.indentColumn := 0
    def lxr = lexer.lexer(code)
    def body = parseObjectBody(lxr)
    ast.objectConstructor(body, ast.nil)
}

method parseModule(module, code) {
    lexer.modulePrefix := module ++ ":"
    lexer.indentColumn := 0
    def lxr = lexer.lexer(code)
    def body = parseObjectBody(lxr)
    ast.objectConstructor(body, ast.nil)
}
