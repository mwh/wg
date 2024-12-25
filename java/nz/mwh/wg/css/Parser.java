package nz.mwh.wg.css;

public class Parser {
    
    private String source;
    private int index = 0;

    public Parser(String source) {
        this.source = source;
    }

    public Rule parseRule() {
        Selector selector = parseSelectorSequence();
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length() || source.charAt(index) != '{') {
            throw new Error("Expected '{' at index " + index);
        }
        index++;
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length()) {
            throw new Error("Expected '}' at end of rule");
        }
        return new Rule(selector);
    }

    public Selector parseSelectorSequence() {
        Selector baseSelector = parseSelector();
        Selector selector = baseSelector;
        Combinator combinator = parseCombinator();
        while (combinator != null) {
            selector.addSuccessor(combinator);
            selector = combinator;
            combinator = parseCombinator();
        }
        return baseSelector;
    }

    public Selector parseSelector() {
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length()) {
            return null;
        }
        char c = source.charAt(index);
        if (c == ':') {
            index++;
            return parsePseudoclassSelector();
        } else if (c == '.') {
            index++;
            return parseStackFrameSelector();
        } else {
            return parseSyntaxSelector();
        }
    }

    public Combinator parseCombinator() {
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length()) {
            return null;
        }
        char c = source.charAt(index);
        if (c == '>') {
            index++;
            return new ChildCombinator(parseSelector());
        // } else if (c == '+') {
        //     index++;
        //     return new AdjacentSiblingCombinator(before, parseSelector());
        // } else if (c == '~') {
        //     index++;
        //     return new GeneralSiblingCombinator(before, parseSelector());
        } else if (c == '{') {
            return null;
        } else if (isIdentifierCharacter(c)) {
            return new DescendantCombinator(parseSelector());
        } else if (c == '.') {
            index++;
            return new DescendantCombinator(parseStackFrameSelector());
        } else {
            throw new Error("Unexpected character '" + c + "' at index " + index);
        }

    }

    private boolean isWhitespace(char c) {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    private boolean isIdentifierCharacter(char c) {
        return Character.isLetterOrDigit(c) || c == '-' || c == '_';
    }

    private PseudoclassSelector parsePseudoclassSelector() {
        int start = index;
        while (index < source.length() && isIdentifierCharacter(source.charAt(index))) {
            index++;
        }
        String pseudoclass = source.substring(start, index);
        String arg = null;
        if (index < source.length() && source.charAt(index) == '(') {
            index++;
            int startArg = index;
            while (index < source.length() && source.charAt(index) != ')') {
                index++;
            }
            if (index >= source.length()) {
                throw new Error("Expected ')' at end of pseudoclass");
            }
            index++;
            arg = source.substring(startArg, index - 1);
        }
        return new PseudoclassSelector(pseudoclass, arg);
    }

    private PseudoElementSelector parsePseudoElementSelector() {
        int start = index;
        while (index < source.length() && isIdentifierCharacter(source.charAt(index))) {
            index++;
        }
        String pseudoElement = source.substring(start, index);
        return new PseudoElementSelector(pseudoElement);
    }

    private StackFrameSelector parseStackFrameSelector() {
        int start = index;
        while (index < source.length() && isIdentifierCharacter(source.charAt(index))) {
            index++;
        }
        String methodName = source.substring(start, index);
        return new StackFrameSelector(methodName);
    }

    private SyntaxSelector parseSyntaxSelector() {
        int start = index;
        while (index < source.length() && Character.isLetterOrDigit(source.charAt(index))) {
            index++;
        }
        String syntax = source.substring(start, index);
        SyntaxSelector result = new SyntaxSelector(syntax);
        while (index < source.length() && source.charAt(index) == ':') {
            index++;
            if (index < source.length() && source.charAt(index) == ':') {
                index++;
                result.setPseudoElement(parsePseudoElementSelector());
            } else {
                result.addPseudoclass(parsePseudoclassSelector());
            }
        }
        return result;
    }
}
