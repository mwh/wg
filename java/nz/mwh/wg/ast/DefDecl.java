package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class DefDecl extends ASTNode {
    String name;
    ASTNode type;
    Cons<String> annotations;
    ASTNode value;

    public DefDecl(String name, ASTNode type, Cons<String> annotations,  ASTNode value) {
        this.name = name;
        this.type = type;
        this.annotations = annotations;
        this.value = value;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "defDecl(\"" + name + "\", " + (type == null ? "nil" : type) + ", " + annotations + ", " + value + ")";
    }

    public String getName() {
        return name;
    }

    public Cons<String> getAnnotations() {
        return annotations;
    }

    public ASTNode getValue() {
        return value;
    }
}
