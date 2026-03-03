package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class Part extends ASTNode {
    String name;
    List<? extends ASTNode> parameters;
    List<? extends ASTNode> genericParameters;

    public Part(String name, Cons<? extends ASTNode> parameters) {
        this.name = name;
        this.parameters = parameters.toList();
    }

    public Part(String name, Cons<? extends ASTNode> parameters, Cons<? extends ASTNode> genericParameters) {
        this.name = name;
        this.parameters = parameters.toList();
        this.genericParameters = genericParameters.toList();
    }
    
    public <T> T accept(T context, Visitor<T> visitor) {
        throw new RuntimeException("Cannot visit DeclarationPart");
    }

    public String toString() {
        // if (genericParameters == null || genericParameters.isEmpty()) {
        //     return "part(\"" + name + "\", " + Cons.stringFromList(parameters) + ")";
        // }
        return "part(\"" + name + "\", " + Cons.stringFromList(parameters) + ", " + Cons.stringFromList(genericParameters) + ")";
    }

    public String getName() {
        return name;
    }

    public List<? extends ASTNode> getParameters() {
        return parameters;
    }

    public List<? extends ASTNode> getArgs() {
        return parameters;
    }

}
