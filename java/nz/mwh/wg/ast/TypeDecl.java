package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class TypeDecl extends ASTNode {
    private final String name;
    private final ASTNode type;

    public TypeDecl(String name, ASTNode type) {
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public ASTNode getType() {
        return type;
    }

    @Override
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    @Override
    public String toString() {
        return "typeDecl(" + name + ", " + (type == null ? "nil" : "one(" + type + ")") + ")";
    }

}