package nz.mwh.wg.runtime;

import java.util.HashSet;
import java.util.Set;

public class GraceExceptionKind implements GraceObject {
    private final String name;
    private final GraceExceptionKind parent;
    private final Set<GraceExceptionKind> descendants = new HashSet<>();

    public static final GraceExceptionKind BASE = new GraceExceptionKind("Exception");

    public GraceExceptionKind(String name) {
        this.name = name;
        this.parent = BASE;
        addDescendant(this);
    }

    public GraceExceptionKind(GraceExceptionKind parent, String name) {
        this.name = name;
        this.parent = parent;
        addDescendant(this);
    }

    @Override
    public GraceObject request(Request request) {
        switch (request.getName()) {
            case "asString(0)":
                return new GraceString("Exception[" + name + "]");
            case "refine(1)":
                var child = new GraceExceptionKind(this, request.getParts().get(0).getArgs().get(0).toString());
                addDescendant(child);
                return child;
            case "raise(1)":
                throw new GraceException(request.getVisitor(), this, request.getParts().get(0).getArgs().get(0).toString());
            case "match(1)":
                GraceObject obj = request.getParts().get(0).getArgs().get(0);
                if (obj instanceof GraceException ex) {
                    if (ex.getKind() == this) {
                        return new GraceMatchResult(true, obj);
                    } else if (descendants.contains(ex.getKind())) {
                        return new GraceMatchResult(true, obj);
                    } else {
                        return new GraceMatchResult(false, obj);
                    }
                } else {
                    return new GraceMatchResult(false, obj);
                }
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

    @Override
    public String toString() {
        return "ExceptionKind[" + name + "]";
    }

    private void addDescendant(GraceExceptionKind child) {
        descendants.add(child);
        if (parent != null) {
            parent.addDescendant(child);
        }
    }
    
}
