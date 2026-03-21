package nz.mwh.wg.runtime;

import java.util.List;

public class GraceBinding implements GraceObject {
    private GraceObject key;
    private GraceObject value;

    public GraceBinding(GraceObject key, GraceObject value) {
        this.key = key;
        this.value = value;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(key.toString());
        sb.append("::");
        sb.append(value.toString());
        return sb.toString();
    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        switch(request.getName()) {
            case "asString(0)":
                return new GraceString(key.toString() + "::" + value.toString());
            case "asDebugString(0)":
                return new GraceString("binding(" + key.toString() + "::" + value.toString() + ")");
            case "key(0)":
                return key;
            case "value(0)":
                return value;
            case "==(1)":
                GraceObject other = parts.get(0).getArgs().get(0);
                if (other instanceof GraceBinding otherBinding) {
                    Request keyEq = Request.unary(request.getVisitor(), "==", otherBinding.key);
                    Request valueEq = Request.unary(request.getVisitor(), "==", otherBinding.value);
                    return key.request(keyEq).request(Request.unary(request.getVisitor(), "&&", value.request(valueEq)));
                } else {
                    return new GraceBoolean(false);
                }
            case "!=(1)":
                GraceObject eq = request(Request.unary(request.getVisitor(), "==", parts.get(0).getArgs().get(0)));
                return eq.request(Request.nullary(request.getVisitor(), "prefix!"));
            case "::(1)":
                return new GraceBinding(this, parts.get(0).getArgs().get(0));
        }
        throw new GraceException(request.getVisitor(), "No such method in Binding: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}
