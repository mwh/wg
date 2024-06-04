package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class Block extends ASTNode {
    Cons<ASTNode> parameters;
    Cons<ASTNode> body;

    public Block(Cons<ASTNode> parameters, Cons<ASTNode> body) {
        this.parameters = parameters;
        this.body = body;
    }

    
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "block(" + parameters + ", " + body + ")";
    }

    public Cons<ASTNode> getParameters() {
        return parameters;
    }

    public Cons<ASTNode> getBody() {
        return body;
    }
}
