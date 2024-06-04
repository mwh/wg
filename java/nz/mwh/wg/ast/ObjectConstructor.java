package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class ObjectConstructor extends ASTNode {
    Cons<ASTNode> body;

    public ObjectConstructor(Cons<ASTNode> body) {
        this.body = body;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "objectConstructor(" + body + ")";
    }

    public Cons<ASTNode> getBody() {
        return body;
    }
}