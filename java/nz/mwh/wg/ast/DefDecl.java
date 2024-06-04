package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class DefDecl extends ASTNode {
    String name;
    Cons<String> annotations;
    ASTNode value;

    public DefDecl(String name, Cons<String> annotations,  ASTNode value) {
        this.name = name;
        this.annotations = annotations;
        this.value = value;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "defDecl(\"" + name + "\", " + annotations + ", " + value + ")";
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
