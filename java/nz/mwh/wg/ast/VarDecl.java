package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class VarDecl extends ASTNode {
    String name;
    ASTNode type;
    List<String> annotations;
    ASTNode value;

    public VarDecl(String name, ASTNode type, Cons<String> annotations, ASTNode value) {
        this.name = name;
        this.type = type;
        this.annotations = annotations.toList();
        this.value = value;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "varDecl(\"" + name + "\", " + (type == null ? "nil" : type) + ", " + Cons.stringFromList(annotations) + ", " + value + ")";
    }

    public String getName() {
        return name;
    }

    public List<String> getAnnotations() {
        return annotations;
    }

    public ASTNode getValue() {
        return value;
    }
}
