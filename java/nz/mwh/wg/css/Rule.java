package nz.mwh.wg.css;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Assign;
import nz.mwh.wg.ast.LexicalRequest;
import nz.mwh.wg.ast.MethodDecl;
import nz.mwh.wg.ast.ObjectConstructor;
import nz.mwh.wg.ast.VarDecl;
import nz.mwh.wg.runtime.GraceObject;

public class Rule {
    private Selector selector;
    private List<Property> body;
    private List<Trace> traces;
    private Rule baseRule;
    
    public Rule(Selector selector) {
        this(selector, new ArrayList<Property>());
    }

    public Rule(Selector selector, List<Property> body) {
        this(selector, body, new ArrayList<Trace>(), null);
    }

    private Rule(Selector selector, List<Property> body, List<Trace> traces, Rule baseRule) {
        this.selector = selector;
        this.body = body;
        this.traces = traces;
        this.baseRule = baseRule == null ? this : baseRule;
    }

    private Rule(Selector selector, Rule baseRule) {
        this.selector = selector;
        this.body = baseRule.body;
        this.traces = baseRule.traces;
        this.baseRule = baseRule;
    }

    public Selector getSelector() {
        return selector;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(selectorString());
        sb.append(" {");
        for (Property p : body) {
            sb.append(p.toString());
            sb.append(";");
        }
        sb.append("}");
        return sb.toString();
    }

    public String selectorString() {
        String result = selector.toString();
        Selector s = selector.getNext();
        while (s != null) {
            result += s.toString();
            s = s.getNext();
        }
        return result;
    }

    public List<Rule> successors(VarDecl varDecl, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(varDecl, scope)) {
            Rule r = new Rule(s, baseRule);
            result.add(r);
        }
        return result;
    }

    public List<Rule> successors(MethodDecl methodDecl, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(methodDecl, scope)) {
            Rule r = new Rule(s, baseRule);
            result.add(r);
        }
        return result;
    }

    
    public List<Rule> successors(ObjectConstructor objCons, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(objCons, scope)) {
            Rule r = new Rule(s, baseRule);
            result.add(r);
        }
        return result;
    }

        
    public List<Rule> successors(Assign assign, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(assign, scope)) {
            Rule r = new Rule(s, baseRule);
            result.add(r);
        }
        return result;
    }

    public List<Rule> frameSuccessors(String name, MethodDecl node, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.frameSuccessors(name, node, scope)) {
            Rule r = new Rule(s, baseRule);
            result.add(r);
        }
        return result;
    }

    public void execute(Assign assign, GraceObject scope, GraceObject value) {
        if (selector.match(assign, scope)) {
            String name;
            if (assign.getTarget() instanceof LexicalRequest target) {
                name = target.getParts().get(0).getName();
            } else {
                name = assign.getTarget().toString();
            }
            if (selector.getPseudoElement() != null) {
                PseudoElementSelector pseudoElement = selector.getPseudoElement();
                if ("value".equals(pseudoElement.getElement())) {
                    tracePseudoElement("%2$s", assign, "value", Map.of(), value);
                    return;
                } else if ("name".equals(pseudoElement.getElement())) {
                    tracePseudoElement("%2$s", assign, "name", Map.of(), name);
                    return;
                } else {
                    return;
                }
            }
            traceNode(name + " := %2$s", assign, Map.of("value", value, "name", name, "left", name, "right", value), value);
        }
    }

    
    public void execute(VarDecl varDecl, GraceObject scope, GraceObject value) {
        if (selector.match(varDecl, scope)) {
            if (selector.getPseudoElement() != null) {
                PseudoElementSelector pseudoElement = selector.getPseudoElement();
                if ("value".equals(pseudoElement.getElement())) {
                    tracePseudoElement("%2$s", varDecl, "value", Map.of(), value);
                    return;
                } else if ("name".equals(pseudoElement.getElement())) {
                    tracePseudoElement("%2$s", varDecl, "name", Map.of(), varDecl.getName());
                    return;
                } else {
                    return;
                }
            }
            traceNode("var " + varDecl.getName() + " := %2$s", varDecl, Map.of("value", value, "name", varDecl.getName()), value);
        }
    }

    private void tracePseudoElement(String defaultFormat, ASTNode node, String element, Map<String, Object> attributes, Object value) {
        var cp = new PropertyComputer(this.body, node, attributes, value, defaultFormat);
        var trace = new Trace(node, cp, baseRule);
        traces.add(trace);
        System.out.println("::> " + trace);
    }
    
    private void traceNode(String defaultFormat, ASTNode node, Map<String, Object> attributes, GraceObject value) {
        var cp = new PropertyComputer(this.body, node, attributes, value, defaultFormat);
        var trace = new Trace(node, cp, baseRule);
        traces.add(trace);
        System.out.println("::> " + trace);
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Rule) {
            Rule other = (Rule) obj;
            return this.toString().equals(other.toString());
        }
        return false;
    }

    @Override
    public int hashCode() {
        return this.toString().hashCode();
    }
}
