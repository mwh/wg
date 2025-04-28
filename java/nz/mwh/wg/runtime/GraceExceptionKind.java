package nz.mwh.wg.runtime;

public class GraceExceptionKind implements GraceObject {
    private final String name;
    private final GraceExceptionKind parent;

    public static final GraceExceptionKind BASE = new GraceExceptionKind("Error");

    public GraceExceptionKind(String name) {
        this.name = name;
        this.parent = BASE;
    }

    public GraceExceptionKind(GraceExceptionKind parent, String name) {
        this.name = name;
        this.parent = parent;
    }

    @Override
    public GraceObject request(Request request) {
        switch (request.getName()) {
            case "asString(0)":
                return new GraceString("Exception[" + name + "]");
            case "refine(1)":
                return new GraceExceptionKind(this, request.getParts().get(0).getArgs().get(0).toString());
            case "raise(1)":
                throw new GraceException(request.getVisitor(), this, request.getParts().get(0).getArgs().get(0).toString());
        }
        throw new GraceException(request.getVisitor(), "No such method in ExceptionKind: " + request.getName());
    }

    public GraceExceptionKind getParent() {
        return parent;
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }

    public String getName() {
        return name;
    }
    
}
