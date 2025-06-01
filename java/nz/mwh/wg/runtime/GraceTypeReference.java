package nz.mwh.wg.runtime;

public class GraceTypeReference implements GraceObject {

    private String name;
    private GraceObject type;

    public GraceTypeReference(String name, GraceObject type) {
        this.name = name;
        this.type = type;
    }

    public void setType(GraceObject type) {
        this.type = type;
    }

    public String getName() {
        return name;
    }

    public GraceObject getType() {
        return type;
    }

    @Override
    public GraceObject request(Request request) {
        switch (request.getName()) {
            case "|(1)":
                return new GracePatternOr(this, request.getParts().get(0).getArgs().get(0));
        }
        if (type != null) {
            return type.request(request);
        } else {
            throw new RuntimeException("Type reference '" + name + "' is null, cannot process request: " + request.getName());
        }
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new UnsupportedOperationException("Unimplemented method 'findReceiver'");
    }

    @Override
    public String toString() {
        if (type == null) {
            return "<Uninitialised type reference " + name + ">";
        }
        return type.toString();
    }

}