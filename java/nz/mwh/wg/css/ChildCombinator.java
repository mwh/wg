package nz.mwh.wg.css;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.runtime.GraceObject;

public class ChildCombinator extends Combinator {
    private Selector child;

    public ChildCombinator(Selector child) {
        this.child = child;
    }

    public Selector getChild() {
        return child;
    }

    public String toString() {
        return " > " + child.toString();
    }

    public boolean matchAt(ASTNode node, GraceObject scope) {
        return child.matchAt(node, scope);
    }

    public List<Selector> successors(ASTNode node, GraceObject scope) {
        // System.out.println("child check " + node);
        if (child.matchAt(node, scope)) {
            // System.out.println("child match " + node);
            return super.successors(node, scope);
        }
        // System.out.println("child no match " + node);
        return new ArrayList<>();
    }

    
    @Override
    public boolean matchFrameAt(String name, ASTNode node, GraceObject scope) {
        return child.matchFrameAt(name, node, scope);
    }

    @Override
    public List<Selector> frameSuccessors(String name, ASTNode node, GraceObject scope) {
        if (child.matchFrameAt(name, node, scope)) {
            return super.frameSuccessors(name, node, scope);
        }
        return new ArrayList<>();
    }

    public PseudoElementSelector getPseudoElement() {
        return child.getPseudoElement();
    }
}
