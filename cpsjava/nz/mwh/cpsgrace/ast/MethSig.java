package nz.mwh.cpsgrace.ast;

import java.util.List;

public class MethSig extends ASTNode {
    private final List<Part> parts;
    private final ASTNode returnType;
    private final String name;

    public MethSig(List<Part> parts, ASTNode returnType) {
        this.parts = parts;
        this.returnType = returnType;
        StringBuilder sb = new StringBuilder();
        for (Part p : parts) {
            sb.append(p.getName());
            int argCount = p.getArguments().size();
            if (argCount > 0) {
                sb.append('(');
                sb.append(argCount);
                sb.append(')');
            }
        }
        this.name = sb.toString();
    }

    public String getName() {
        return name;
    }

    public List<Part> getParts() {
        return parts;
    }

    public ASTNode getReturnType() {
        return returnType;
    }
}
