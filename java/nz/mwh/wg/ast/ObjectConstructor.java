package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class ObjectConstructor extends ASTNode {
    List<ASTNode> body;

    public ObjectConstructor(Cons<ASTNode> body) {
        this.body = body.toList();
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "objectConstructor(" + Cons.stringFromList(body) + ")";
    }

    public List<ASTNode> getBody() {
        return body;
    }
}