package nz.mwh.wg.css;

import java.util.ArrayList;
import java.util.List;

public class Parser {
    
    private String source;
    private int index = 0;

    public Parser(String source) {
        this.source = source;
    }

    public List<Rule> parseRules() {
        List<Rule> rules = new ArrayList<>();
        while (index < source.length()) {
            Rule rule = parseRule();
            rules.add(rule);
            if (index < source.length() && (source.charAt(index) == ';' || source.charAt(index) == '}')) {
                index++;
            }
        }
        return rules;
    }

    public Rule parseRule() {
        Selector selector = parseSelectorSequence();
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length() || source.charAt(index) != '{') {
            if (index == source.length() || source.charAt(index) == ';') { // extension; allow rules without body
                return new Rule(selector);
            }
            throw new Error("Expected '{' at index " + index);
        }
        index++;
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        var body = parseBody();
        if (index >= source.length()) {
            throw new Error("Expected '}' at end of rule");
        }
        return new Rule(selector, body);
    }

    private List<Property> parseBody() {
        var properties = new ArrayList<Property>();
        while (index < source.length() && source.charAt(index) != '}') {
            Property property = parseProperty();
            properties.add(property);
            while (index < source.length() && isWhitespace(source.charAt(index))) {
                index++;
            }
        }
        return properties;
    }

    private Property parseProperty() {
        int start = index;
        while (index < source.length() && isIdentifierCharacter(source.charAt(index))) {
            index++;
        }
        String name = source.substring(start, index);
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length() || source.charAt(index) != ':') {
            throw new Error("Expected ':' at index " + index);
        }
        index++;
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        return parsePropertyValue(name);
    }

    private Property parsePropertyValue(String propertyName) {
        List<Property.Value> values = new ArrayList<>();
        while (index < source.length() && source.charAt(index) != ';' && source.charAt(index) != '}') {
            Property.Value value = parseSingleValue();
            values.add(value);
            while (index < source.length() && isWhitespace(source.charAt(index))) {
                index++;
            }
        }
        if (index < source.length() && source.charAt(index) == ';')
            index++;
        return new Property(propertyName, values);
    }

    private Property.Value parseSingleValue() {
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        if (index >= source.length()) {
            throw new Error("Expected value at end of property");
        }
        char c = source.charAt(index);
        if (c == '"') {
            return parseString();
        } else if (c == '\'') {
            return parseString();
        } else if (Character.isDigit(c)) {
            return parseNumber();
        } else {
            return parseIdentifier();
        }
    }

    private Property.StringLiteral parseString() {
        int start = index;
        char quote = source.charAt(index);
        index++;
        while (index < source.length() && source.charAt(index) != quote) {
            index++;
        }
        if (index >= source.length()) {
            throw new Error("Expected closing quote at end of string");
        }
        index++;
        return new Property.StringLiteral(source.substring(start + 1, index - 1));
    }

    private Property.Number parseNumber() {
        int start = index;
        while (index < source.length() && Character.isDigit(source.charAt(index))) {
            index++;
        }
        if (index < source.length() && source.charAt(index) == '.') {
            index++;
            while (index < source.length() && Character.isDigit(source.charAt(index))) {
                index++;
            }
        }
        return new Property.Number(Double.parseDouble(source.substring(start, index)));
    }

    private Property.Value parseIdentifier() {
        int start = index;
        while (index < source.length() && isIdentifierCharacter(source.charAt(index))) {
            index++;
        }
        while (index < source.length() && isWhitespace(source.charAt(index))) {
            index++;
        }
        String name = source.substring(start, index);
        if (index < source.length() && source.charAt(index) == '(') {
            return parseFunction(name);
        }
        return new Property.Keyword(name);
    }

    public Property.Function parseFunction(String name) {
        List<Property.Value> arguments = new ArrayList<>();
        index++;
        while (index < source.length() && source.charAt(index) != ')') {
            Property.Value value = parseSingleValue();
            arguments.add(value);
            while (index < source.length() && isWhitespace(source.charAt(index))) {
                index++;
            }
        }
        if (index >= source.length()) {
            throw new Error("Expected ')' at end of function");
        }
        index++;
        return new Property.Function(name, arguments);
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
        } else if (c == '{' || c == ';') {
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
