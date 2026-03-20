var modulePrefix := ""
var indentColumn := 0

method EOFToken(line, column) {
    SymbolToken(line, column, "EOF")
}

method NumberToken(ln, col, val) {
    object {
        def nature is public = "NUMBER"
        def value is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ value ++ ")[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method LParenToken(line, column) {
    SymbolToken(line, column, "LPAREN")
}

method RParenToken(line, column) {
    SymbolToken(line, column, "RPAREN")
}

method LGenericToken(line, column) {
    SymbolToken(line, column, "LGENERIC")
}

method RGenericToken(line, column) {
    SymbolToken(line, column, "RGENERIC")
}

method LBraceToken(line, column) {
    SymbolToken(line, column, "LBRACE")
}

method RBraceToken(ln, col, idx) {
    object {
        def nature is public = "RBRACE"
        def line is public = ln
        def column is public = col
        def index is public = idx

        method asString {
            "RBRACE[" ++ line ++ ":" ++ column ++ "@" ++ index ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }

    }
}

method LSquareToken(line, column) {
    SymbolToken(line, column, "LSQUARE")
}

method RSquareToken(line, column) {
    SymbolToken(line, column, "RSQUARE")
}

method CommaToken(line, column) {
    SymbolToken(line, column, "COMMA")
}

method DotToken(line, column) {
    SymbolToken(line, column, "DOT")
}

method IdentifierToken(ln, col, val) {
    object {
        def nature is public = "IDENTIFIER"
        def value is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ value ++ ")[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method KeywordToken(ln, col, val) {
    object {
        def nature is public = "KEYWORD"
        def value is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method OperatorToken(ln, col, val) {
    object {
        def nature is public = "OPERATOR"
        def value is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method StringToken(ln, col, val) {
    object {
        def nature is public = "STRING"
        def value is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method InterpStringToken(ln, col, val) {
    object {
        def nature is public = "INTERPSTRING"
        def value is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ value ++ ")" ++ "[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method EqualsToken(line, column) {
    SymbolToken(line, column, "EQUALS")
}

method AssignToken(line, column) {
    SymbolToken(line, column, "ASSIGN")
}

method ArrowToken(line, column) {
    SymbolToken(line, column, "ARROW")
}

method SymbolToken(ln, col, nat) {
    object {
        def nature is public = nat
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method NewlineToken(line, column) {
    SymbolToken(line, column, "NEWLINE")
}

method CommentToken(ln, col, text) {
    object {
        def nature is public = "COMMENT"
        def line is public = ln
        def column is public = col
        def value is public = text

        method asString {
            nature ++ "(" ++ value ++ ")[" ++ line ++ ":" ++ column ++ "]"
        }

        method location {
            modulePrefix ++ line ++ ":" ++ column
        }
    }
}

method isOperatorCharacter(c) {
    (c == "+") || (c == "-") || (c == "*") || (c == "/") || (c == "=") || (c == ":") || (c == "|") || (c == "&") || (c == "!") || (c == ">") || (c == "<") || (c == ".") || (c == "%")
}

method isIdentifierStart(c) {
    def cp = c.firstCodepoint
    ((cp >= 97) && (cp <= 122)) || ((cp >= 65) && (cp <= 90)) || (cp == 95)
}

method isDigit(c) {
    def cp = c.firstCodepoint
    (cp >= 48) && (cp <= 57)
}

method ErrorToken(ln, col, val) {
    object {
        def nature is public = "ERROR"
        def message is public = val
        def line is public = ln
        def column is public = col

        method asString {
            nature ++ "(" ++ message ++ ")[" ++ line ++ ":" ++ column ++ "]"
        }
    }
}

method lexer(code) {
    object {
        def source = code
        var index := 1
        var line := 1
        var column := 0
        var lineStart := 0
        var currentToken := nextToken

        method save {
            object {
                def i = index
                def l = line
                def c = column
                def s = lineStart
                def t = currentToken
            }
        }

        method restore(memo) {
            index := memo.i
            line := memo.l
            column := memo.c
            lineStart := memo.s
            currentToken := memo.t
        }

        method nextToken {
            if (index > source.size) then {
                return EOFToken(line, column)
            }

            var c := source.at(index)
            column := index - lineStart
            index := index + 1
            

            if (c == " ") then {
                return nextToken
            }

            if (c == "(") then {
                return LParenToken(line, column)
            }

            if (c == ")") then {
                return RParenToken(line, column)
            }

            if (c == "[") then {
                if (index <= source.size) then {
                    def nextChar = source.at(index)
                    if (nextChar == "[") then {
                        index := index + 1
                        return LGenericToken(line, column)
                    }
                }
                return LSquareToken(line, column)
            }

            if (c == "]") then {
                if (index <= source.size) then {
                    def nextChar = source.at(index)
                    if (nextChar == "]") then {
                        index := index + 1
                        return RGenericToken(line, column)
                    }
                }
                return RSquareToken(line, column)
            }

            if ((c > "z") && (c < "|")) then {
                return LBraceToken(line, column)
            }

            if (c == "}") then {
                return RBraceToken(line, column, index)
            }

            if (c == ",") then {
                return CommaToken(line, column)
            }

            if (c.firstCodepoint == 13) then {
                c := source.at(index)
                index := index + 1
            }
            if ((c.firstCodepoint == 10) || (c.firstCodepoint == 8232)) then {
                line := line + 1
                lineStart := index - 1
                return NewlineToken(line, column)
            }

            if (c.firstCodepoint == 34) then {
                return lexString
            }

            if (isDigit(c)) then {
                def startIndex = index - 1
                if (index >= source.size) then {
                    return NumberToken(line, column, c)
                }
                var value := ""
                while {isDigit(c) && (index <= source.size)} do {
                    value := value ++ c
                    c := source.at(index)
                    index := index + 1
                }
                if (c == ".") then {
                    if (isDigit(source.at(index))) then {
                        value := value ++ c
                        c := source.at(index)
                        index := index + 1
                        while {isDigit(c) && (index <= source.size)} do {
                            value := value ++ c
                            c := source.at(index)
                            index := index + 1
                        }
                    }
                }
                if (index > (startIndex + 1)) then {
                    index := index - 1
                }
                return NumberToken(line, column, value)
            }

            if (isIdentifierStart(c)) then {
                def startIndex = index - 1
                var value := c
                if (index > source.size) then {
                    return IdentifierToken(line, column, value)
                }
                c := source.at(index)
                while {(isIdentifierStart(c) || isDigit(c) || (c == "'")) && (index <= source.size)} do {
                    value := value ++ c
                    index := index + 1
                    if (index <= source.size) then {
                        c := source.at(index)
                    }
                }
                if ((value == "var") || (value == "def") || (value == "method") || (value == "object") || (value == "is") || (value == "return") || (value == "class") || (value == "type") || (value == "import") || (value == "self") || (value == "dialect") || (value == "interface") || (value == "inherit") || (value == "use")) then {
                    return KeywordToken(line, column, value)
                }
                return IdentifierToken(line, column, value)
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
                    return AssignToken(line, column)
                }
                if (op == "=") then {
                    return EqualsToken(line, column)
                }
                if (op == "->") then {
                    return ArrowToken(line, column)
                }
                if (op == ".") then {
                    return DotToken(line, column)
                }
                if (op == ":") then {
                    return SymbolToken(line, column, "COLON")
                }
                if ((op.size >= 2) && (op.substringFrom 1 to 2 == "//")) then {
                    var cp := c.firstCodepoint
                    var text := ""
                    index := index + 1
                    while { (cp != 10) && (cp != 13) && (index <= source.size) } do {
                        text := text ++ c
                        c := source.at(index)
                        cp := c.firstCodepoint
                        index := index + 1
                    }
                    if ((cp == 10) || (cp == 13)) then {
                        index := index - 1
                    }
                    return CommentToken(line, column, text)
                }
                return OperatorToken(line, column, op)
            }

            if (c == ";") then {
                return SymbolToken(line, column, "SEMICOLON")
            }

            parseError(line, column, "Unknown character: " ++ c.asString ++ " (" ++ c.firstCodepoint.asString ++ ")")
        
        }

        method current {
            currentToken
        }

        method advance {
            currentToken := nextToken
            if (currentToken.nature == "NEWLINE") then {
                def pendingToken = peek
                if (pendingToken.column > indentColumn) then {
                    advance
                }
            }
        }

        method peek {
            def oldIndex = index
            def oldLine = line
            def oldLineStart = lineStart
            def oldColumn = column
            def oldCurrentToken = currentToken

            advance
            def pending = currentToken

            index := oldIndex
            line := oldLine
            lineStart := oldLineStart
            column := oldColumn
            currentToken := oldCurrentToken
            
            return pending
        }

        method lexString {
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
                    if ((cp == 123) && (!escaped)) then {
                        // String interpolation
                        index := index + 1
                        return InterpStringToken(line, column, value)
                    } else {
                        value := value ++ source.at(index)
                    }
                }
                escaped := escapeNext
                index := index + 1
            }
            index := index + 1
            return StringToken(line, column, value)
        }

        method startStringAt(pos) {
            index := pos
            currentToken := lexString
        }

        method windback(pos) {
            index := pos
        }

        method expectToken(nature) {
            if (currentToken.nature != nature) then {
                print("Expected " ++ nature ++ " but got " ++ currentToken.nature ++ " at " ++ currentToken.line ++ ":" ++ currentToken.column)
                parseError(currentToken.line, currentToken.column, "Expected " ++ nature ++ " but got " ++ currentToken.nature)
            }
        }

        method skipWhitespace {
            while { currentToken.nature == "NEWLINE" } do {
                advance
            }
        }

        method expectSymbol(nature) {
            if (currentToken.nature != nature) then {
                parseError(currentToken.line, currentToken.column, "Expected " ++ nature ++ " but got " ++ currentToken.nature)
            }
        }

        method expectSymbol(nature) explaining(msg) {
            if (currentToken.nature != nature) then {
                parseError(currentToken.line, currentToken.column, msg ++ " Expected " ++ nature ++ " but got " ++ currentToken.nature)
            }
        }

        method expectSymbol(nature) or(nature2) explaining(msg) {
            if ((currentToken.nature != nature) && (currentToken.nature != nature2)) then {
                parseError(currentToken.line, currentToken.column, msg ++ " Expected " ++ nature ++ " or " ++ nature2 ++ " but got " ++ currentToken.nature)
            }
        }

        method expectKeyword(val) {
            if (currentToken.nature != "KEYWORD") then {
                parseError(currentToken.line, currentToken.column, "Expected KEYWORD but got " ++ currentToken.nature)
            }
            if (currentToken.value != val) then {
                parseError(currentToken.line, currentToken.column, "Expected " ++ val ++ " but got " ++ currentToken.value)
            }
        }
    }
}

method digitToNumber(token, c) {
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
    parseError(token.line, token.column, "Unexpected digit: " ++ c.asString)
}


method parseError(line, column, message) {
    print("Lexical error: " ++ message ++ " at " ++ modulePrefix ++ line ++ ":" ++ column)
    Exception.refine "LexicalError".raise(modulePrefix ++ line ++ ":" ++ column ++ ": " ++ message)
}
