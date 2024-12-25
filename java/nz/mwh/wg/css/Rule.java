package nz.mwh.wg.css;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Assign;
import nz.mwh.wg.ast.LexicalRequest;
import nz.mwh.wg.ast.MethodDecl;
import nz.mwh.wg.ast.ObjectConstructor;
import nz.mwh.wg.ast.VarDecl;
import nz.mwh.wg.runtime.GraceObject;

public class Rule {
    private Selector selector;
    private List<Object> body;
    
    public Rule(Selector selector) {
        this(selector, new ArrayList<Object>());
    }

    public Rule(Selector selector, List<Object> body) {
        this.selector = selector;
        this.body = body;
    }

    public Selector getSelector() {
        return selector;
    }

    public String toString() {
        String result = selector.toString();
        Selector s = selector.getNext();
        while (s != null) {
            result += s.toString();
            s = s.getNext();
        }
        return result + " {}";
    }

    public List<Rule> successors(VarDecl varDecl, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(varDecl, scope)) {
            Rule r = new Rule(s, body);
            result.add(r);
        }
        return result;
    }

    public List<Rule> successors(MethodDecl methodDecl, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(methodDecl, scope)) {
            Rule r = new Rule(s, body);
            result.add(r);
        }
        return result;
    }

    
    public List<Rule> successors(ObjectConstructor objCons, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(objCons, scope)) {
            Rule r = new Rule(s, body);
            result.add(r);
        }
        return result;
    }

        
    public List<Rule> successors(Assign assign, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.successors(assign, scope)) {
            Rule r = new Rule(s, body);
            result.add(r);
        }
        return result;
    }

    public List<Rule> frameSuccessors(String name, MethodDecl node, GraceObject scope) {
        List<Rule> result = new ArrayList<>();
        for (Selector s : selector.frameSuccessors(name, node, scope)) {
            Rule r = new Rule(s, body);
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
                    System.out.println("::> " + this.selector + " => " + value.toString());
                    return;
                } else if ("name".equals(pseudoElement.getElement())) {
                    System.out.println("::> " + this.selector + " => " + name);
                    return;
                } else {
                    return;
                }
            }
            System.out.println("::> " + this.selector + " => " + name + " := " + value.toString());
        }
    }

    
    public void execute(VarDecl varDecl, GraceObject scope, GraceObject value) {
        if (selector.match(varDecl, scope)) {
            if (selector.getPseudoElement() != null) {
                PseudoElementSelector pseudoElement = selector.getPseudoElement();
                if ("value".equals(pseudoElement.getElement())) {
                    System.out.println("::> " + this.selector + " => " + value.toString());
                    return;
                } else if ("name".equals(pseudoElement.getElement())) {
                    System.out.println("::> " + this.selector + " => " + varDecl.getName());
                    return;
                } else {
                    return;
                }
            }
            System.out.println("::> " + this.selector + " => " + varDecl.getName() + " := " + value.toString());
        }
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
