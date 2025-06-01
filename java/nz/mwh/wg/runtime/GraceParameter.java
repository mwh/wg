package nz.mwh.wg.runtime;

public class GraceParameter implements GraceObject {
    private String name;
    private GraceObject type;

    public GraceParameter(String name, GraceObject type) {
        this.name = name;
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public GraceObject getType() {
        return type;
    }

    public void setType(GraceObject type) {
        this.type = type;
    }

    @Override
    public String toString() {
        if (type == null) {
            return name;
        }
        if (type instanceof GraceTypeReference typeRef) {
            return name + " : " + typeRef.getName();
        } else {
            return name + " : " + type.toString();
        }
    }

    @Override
    public GraceObject request(Request request) {
        throw new UnsupportedOperationException("Unimplemented method 'request'");
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new UnsupportedOperationException("Unimplemented method 'findReceiver'");
    }
}
