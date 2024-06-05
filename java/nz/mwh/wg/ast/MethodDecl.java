package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;


public class MethodDecl extends ASTNode {
    List<? extends DeclarationPart> parts;
    ASTNode type;
    List<String> annotations;
    List<? extends ASTNode> body;

    public MethodDecl(Cons<? extends DeclarationPart> parts, ASTNode type, Cons<String> annotations, Cons<? extends ASTNode> body) {
        this.parts = parts.toList();
        this.type = type;
        this.annotations = annotations.toList();
        this.body = body.toList();
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "methodDecl(" + Cons.stringFromList(parts) + ", " + (type == null ? "nil" : "cons(" + type + ", nil)") + ", " + Cons.stringFromList(annotations) + ", " + Cons.stringFromList(body) + ")";
    }

    public List<? extends DeclarationPart> getParts() {
        return parts;
    }

    public List<? extends ASTNode> getBody() {
        return body;
    }
}
