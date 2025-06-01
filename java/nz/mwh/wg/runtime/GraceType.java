package nz.mwh.wg.runtime;

import java.util.List;
import java.util.stream.Collectors;

import nz.mwh.wg.ast.IdentifierDeclaration;
import nz.mwh.wg.ast.MethodSignature;

public class GraceType  implements GraceObject {

    private List<GraceMethodSignature> methods;

    protected GraceType() {}

    public GraceType(List<GraceMethodSignature> methods) {
        this.methods = methods;
    }

    @Override
    public GraceObject request(Request request) {
        switch(request.getName()) {
            case "asString(0)":
                return new GraceString(toString());
            default:
                throw new RuntimeException("Cannot request method on GraceType: " + request.getName());
        }
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder("interface {");
        for (var method : methods) {
            sb.append("\n  ");
            sb.append(method.toString());
        }
        sb.append("\n}");
        return sb.toString();
    }
    
}
