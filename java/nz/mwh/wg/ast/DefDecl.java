package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class DefDecl extends ASTNode {
    String name;
    ASTNode type;
    List<ASTNode> annotations;
    ASTNode value;
    String pos;

    public DefDecl(String name, ASTNode type, Cons<ASTNode> annotations,  ASTNode value) {
        this(null, name, type, annotations, value);
    }

    public DefDecl(String pos, String name, ASTNode type, Cons<ASTNode> annotations,  ASTNode value) {
        this.name = name;
        this.type = type;
        this.annotations = annotations.toList();
        this.value = value;
        this.pos = pos;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "defDec(\"" + name + "\", " + Cons.fromValue(type) + ", " + Cons.stringFromList(annotations) + ", " + value + ")";
    }

    public String getName() {
        return name;
    }

    public List<ASTNode> getAnnotations() {
        return annotations;
    }

    public ASTNode getValue() {
        return value;
    }
}
