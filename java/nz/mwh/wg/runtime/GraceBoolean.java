package nz.mwh.wg.runtime;

import java.util.List;

public class GraceBoolean implements GraceObject {
    private boolean value;

    public GraceBoolean(boolean value) {
        this.value = value;
    }

    public String toString() {
        return "" + value;
    }

    public boolean getValue() {
        return value;
    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        if (parts.size() == 1) {
            String name = parts.get(0).getName();
            if (name.equals("prefix!") || name.equals("not")) {
                return new GraceBoolean(!value);
            } else if (name.equals("&&")) {
                return new GraceBoolean(value && ((GraceBoolean) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("||")) {
                return new GraceBoolean(value || ((GraceBoolean) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("asString")) {
                return new GraceString("" + value);
            } else if (name.equals("==")) {
                return new GraceBoolean(value == ((GraceBoolean) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("!=")) {
                return new GraceBoolean(value != ((GraceBoolean) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("ifTrue")) {
                if (value) {
                    parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
                }
                return GraceDone.done;
            } else if (name.equals("value")) {
                return this;
            }
        } else if (parts.size() == 2) {
            String name = request.getName();
            if (name.equals("ifTrue(1)ifFalse(1)")) {
                if (value) {
                    return parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
                }
                return parts.get(1).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in Boolean: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }
}
