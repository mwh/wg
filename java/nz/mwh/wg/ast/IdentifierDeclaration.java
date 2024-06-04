package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class IdentifierDeclaration extends ASTNode {
    String name;

    public IdentifierDeclaration(String name) {
        this.name = name;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        throw new RuntimeException("Cannot visit IdentifierDeclaration");
    }

    public String toString() {
        return "identifierDeclaration(\"" + name + "\")";
    }

    public String getName() {
        return name;
    }
}
