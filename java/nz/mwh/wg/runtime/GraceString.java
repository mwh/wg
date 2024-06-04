package nz.mwh.wg.runtime;

import java.util.List;

public class GraceString implements GraceObject {
    private String value;

    public GraceString(String value) {
        this.value = value;
    }

    public String getValue() {
        return value;
    }

    public String toString() {
        return value;
    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        if (parts.size() == 1) {
            String name = parts.get(0).getName();
            if (name.equals("++")) {
                return new GraceString(value + parts.get(0).getArgs().get(0).toString());
            } else if (name.equals("asString")) {
                return new GraceString(value);
            } else if (name.equals("size")) {
                return new GraceNumber(value.length());
            } else if (name.equals("==")) {
                return new GraceBoolean(value.equals(((GraceString) parts.get(0).getArgs().get(0)).value));
            } else if (name.equals("!=")) {
                return new GraceBoolean(!value.equals(((GraceString) parts.get(0).getArgs().get(0)).value));
            } else if (name.equals("<")) {
                return new GraceBoolean(value.compareTo(((GraceString) parts.get(0).getArgs().get(0)).value) < 0);
            } else if (name.equals(">")) {
                return new GraceBoolean(value.compareTo(((GraceString) parts.get(0).getArgs().get(0)).value) > 0);
            } else if (name.equals("at")) {
                int index = (int) ((GraceNumber) parts.get(0).getArgs().get(0)).value;
                return new GraceString("" + value.charAt(index - 1));
            }
        }
        throw new RuntimeException("No such method in String: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}

