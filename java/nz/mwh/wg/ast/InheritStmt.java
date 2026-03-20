package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class InheritStmt extends ASTNode {
    
    private ASTNode parent;
    private List<ASTNode> extra;

    public InheritStmt(ASTNode parent, Cons<ASTNode> extra) {
        this.parent = parent;
        this.extra = extra.toList();
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "inheritStmt(\"" + parent + "\", " + Cons.stringFromList(extra) + ")";
    }

    public ASTNode getExpression() {
        return parent;
    }
}