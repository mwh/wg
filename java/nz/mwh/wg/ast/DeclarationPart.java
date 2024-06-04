package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class DeclarationPart extends ASTNode {
    String name;
    Cons<? extends IdentifierDeclaration> parameters;

    public DeclarationPart(String name, Cons<? extends IdentifierDeclaration> parameters) {
        this.name = name;
        this.parameters = parameters;
    }
    
    public <T> T accept(T context, Visitor<T> visitor) {
        throw new RuntimeException("Cannot visit DeclarationPart");
    }

    public String toString() {
        return "declarationPart(\"" + name + "\", " + parameters + ")";
    }

    public String getName() {
        return name;
    }

    public Cons<? extends IdentifierDeclaration> getParameters() {
        return parameters;
    }
}
