package nz.mwh.cpsgrace.ast;

public class IdentifierDeclaration extends ASTNode {
    private String name;
    private ASTNode type;

    public IdentifierDeclaration(String name, ASTNode type) {
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public ASTNode getType() {
        return type;
    }
}
