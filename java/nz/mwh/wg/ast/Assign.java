package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class Assign extends ASTNode {
    ASTNode target;
    ASTNode value;
    String pos;

    public Assign(ASTNode target, ASTNode value) {
        this(null, target, value);
    }

    public Assign(String pos, ASTNode target, ASTNode value) {
        this.pos = pos;
        this.target = target;
        this.value = value;
    }

    
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "assn(" + target + ", " + value + ")";
    }

    public ASTNode getTarget() {
        return target;
    }

    public ASTNode getValue() {
        return value;
    }
}
