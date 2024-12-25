package nz.mwh.wg.css;


import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.runtime.GraceObject;

public class DescendantCombinator extends Combinator {
    private Selector descendant;

    public DescendantCombinator(Selector descendant) {
        this.descendant = descendant;
    }

    public Selector getDescendant() {
        return descendant;
    }

    public String toString() {
        return " " + descendant.toString();
    }

    @Override
    public boolean matchAt(ASTNode node, GraceObject scope) {
        // System.out.println("descendant check " + node);
        return descendant.matchAt(node, scope);
    }

    @Override
    public boolean matchFrameAt(String name, ASTNode node, GraceObject scope) {
        return descendant.matchFrameAt(name, node, scope);
    }

    public PseudoElementSelector getPseudoElement() {
        return descendant.getPseudoElement();
    }

}
