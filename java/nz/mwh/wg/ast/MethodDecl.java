package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class MethodDecl extends ASTNode {
    Cons<? extends DeclarationPart> parts;
    ASTNode type;
    Cons<String> annotations;
    Cons<? extends ASTNode> body;

    public MethodDecl(Cons<? extends DeclarationPart> parts, ASTNode type, Cons<String> annotations, Cons<? extends ASTNode> body) {
        this.parts = parts;
        this.type = type;
        this.annotations = annotations;
        this.body = body;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "methodDecl(" + parts + ", " + (type == null ? "nil" : "cons(" + type + ", nil)") + ", " + annotations + ", " + body + ")";
    }

    public Cons<? extends DeclarationPart> getParts() {
        return parts;
    }

    public Cons<? extends ASTNode> getBody() {
        return body;
    }
}
