package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class DeclarationPart extends ASTNode {
    String name;
    List<? extends IdentifierDeclaration> parameters;

    public DeclarationPart(String name, Cons<? extends IdentifierDeclaration> parameters) {
        this.name = name;
        this.parameters = parameters.toList();
    }
    
    public <T> T accept(T context, Visitor<T> visitor) {
        throw new RuntimeException("Cannot visit DeclarationPart");
    }

    public String toString() {
        return "declarationPart(\"" + name + "\", " + Cons.stringFromList(parameters) + ")";
    }

    public String getName() {
        return name;
    }

    public List<? extends IdentifierDeclaration> getParameters() {
        return parameters;
    }
}
