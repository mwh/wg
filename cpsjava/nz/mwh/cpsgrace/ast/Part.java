package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

public class Part extends ASTNode {
    private String name;
    private List<ASTNode> arguments;

    public Part(String name, List<ASTNode> arguments) {
        this.name = name;
        this.arguments = new ArrayList<>(arguments);
    }

    public String getName() {
        return name;
    }

    public List<ASTNode> getArguments() {
        return arguments;
    }
}
