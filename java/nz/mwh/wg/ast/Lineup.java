package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class Lineup extends ASTNode {
    private List<ASTNode> elements;

    public Lineup(Cons<ASTNode> elements) {
        this.elements = elements.toList();
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "lineup(" + Cons.stringFromList(elements) + ")";
    }

    public List<ASTNode> getElements() {
        return elements;
    }

}