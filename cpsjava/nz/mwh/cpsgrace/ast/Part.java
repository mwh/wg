package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

public class Part extends ASTNode {
    private String name;
    private List<ASTNode> arguments;
    private List<ASTNode> genericArguments;

    public Part(String name, List<ASTNode> arguments, List<ASTNode> genericArguments) {
        this.name = name;
        this.arguments = new ArrayList<>(arguments);
        this.genericArguments = new ArrayList<>(genericArguments);
    }

    public String getName() {
        return name;
    }

    public List<ASTNode> getArguments() {
        return arguments;
    }

    public List<ASTNode> getGenericArguments() {
        return genericArguments;
    }
}
