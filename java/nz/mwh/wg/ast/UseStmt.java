package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class UseStmt extends ASTNode {
    
    private ASTNode parent;
    private List<ASTNode> extra;
    String pos;

    public UseStmt(ASTNode parent, Cons<ASTNode> extra) {
        this(null, parent, extra);
    }

    public UseStmt(String pos, ASTNode parent, Cons<ASTNode> extra) {
        this.pos = pos;
        this.parent = parent;
        this.extra = extra.toList();
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "useStmt(\"" + parent + "\", " + Cons.stringFromList(extra) + ")";
    }

    public ASTNode getExpression() {
        return parent;
    }
}