package nz.mwh.wg.runtime;

import java.util.List;
import java.util.stream.Collectors;

public class GraceMethodSignature {
    
    private String name;
    private List<RequestPartR> parts;
    private GraceObject returnType;

    public GraceMethodSignature(String name, List<RequestPartR> parts, GraceObject returnType) {
        this.name = name;
        this.parts = parts;
        this.returnType = returnType;
    }

    public String getName() {
        return name;
    }

    public List<RequestPartR> getParts() {
        return parts;
    }

    public GraceObject getReturnType() {
        return returnType;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        for (RequestPartR part : parts) {
            sb.append(part.getName());
            List<GraceObject> args = part.getArgs();
            if (!args.isEmpty()) {
                sb.append("(");
                sb.append(args.stream().map(p -> p.toString()).collect(Collectors.joining(", ")));
                sb.append(")");
            }
            sb.append(" ");
        }
        switch (returnType) {
            case null -> {
                // No return type specified
            }
            case GraceTypeReference typeRef -> {
                sb.append("-> ").append(typeRef.getName()).append(" ");
            }
            default -> {
                sb.append("-> ").append(returnType.toString()).append(" ");
            }
        }
        return sb.toString();
    }
}
