package nz.mwh.wg.runtime;

import java.util.List;

public class GraceNumber implements GraceObject {
    double value;

    public GraceNumber(double value) {
        this.value = value;
    }

    public double getValue() {
        return value;
    }

    public String toString() {
        if (value == (int) value) {
            return ("" + (int) value);
        } else {
            return ("" + value);
        }

    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        if (parts.size() == 1) {
            String name = parts.get(0).getName();
            if (name.equals("+")) {
                return switch (parts.get(0).getArgs().get(0)) {
                    case GraceNumber n -> new GraceNumber(value + n.value);
                    default -> throw new GraceException(request.getVisitor(), "invalid addition argument at " + request.getLocation());
                };
            } else if (name.equals("-")) {
                return new GraceNumber(value - ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("*")) {
                return new GraceNumber(value * ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("/")) {
                return new GraceNumber(value / ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("prefix-")) {
                return new GraceNumber(-value);
            } else if (name.equals(">")) {
                return new GraceBoolean(value > ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("<")) {
                return new GraceBoolean(value < ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals(">=")) {
                return new GraceBoolean(value >= ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("<=")) {
                return new GraceBoolean(value <= ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("==")) {
                return new GraceBoolean(value == ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("!=")) {
                return new GraceBoolean(value != ((GraceNumber) parts.get(0).getArgs().get(0)).value);
            } else if (name.equals("|")) {
                return new GracePatternOr(this, parts.get(0).getArgs().get(0));
            } else if (name.equals("match")) {
                GraceObject target = parts.get(0).getArgs().get(0);
                if (target instanceof GraceNumber num && value == num.value) {
                    return new GraceMatchResult(true, this);
                }
                return new GraceMatchResult(false, target);
            } else if (name.equals("asString")) {
                return new GraceString(toString());
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in Number: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}
