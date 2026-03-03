package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

import java.util.Collections;
import java.util.List;

public class TypeDecl extends ASTNode {
    private final String name;
    private final List<? extends ASTNode> genericParameters;
    private final ASTNode type;

    public TypeDecl(String name, Cons<ASTNode> genericParams, ASTNode type) {
        this.name = name;
        this.genericParameters = genericParams.toList();
        this.type = type;
    }

    public TypeDecl(String name, ASTNode type) {
        this.name = name;
        this.genericParameters = Collections.emptyList();
        this.type = type;
    }

    public String getName() {
        return name;
    }
    
    public List<? extends ASTNode> getGenericParameters() {
        return genericParameters;
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
        return "typeDecl(" + escapeString(name) + ", " + Cons.stringFromList(genericParameters) + ", " + (type == null ? "nil" : "one(" + type + ")") + ")";
    }

}