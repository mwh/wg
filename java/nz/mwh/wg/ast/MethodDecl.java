package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class MethodDecl extends ASTNode {
    Cons<? extends DeclarationPart> parts;
    Cons<? extends ASTNode> body;

    public MethodDecl(Cons<? extends DeclarationPart> parts, Cons<? extends ASTNode> body) {
        this.parts = parts;
        this.body = body;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "methodDecl(" + parts + ", " + body + ")";
    }

    public Cons<? extends DeclarationPart> getParts() {
        return parts;
    }

    public Cons<? extends ASTNode> getBody() {
        return body;
    }
}
