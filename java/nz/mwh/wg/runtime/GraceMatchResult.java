package nz.mwh.wg.runtime;

import java.util.List;

public class GraceMatchResult implements GraceObject {
    private GraceObject value;
    private boolean success;

    public GraceMatchResult(boolean success, GraceObject value) {
        this.success = success;
        this.value = value;
    }

    public String toString() {
        if (success) {
            return "SuccessfulMatch(" + value + ")";
        }
        return "FailedMatch";
    }

    public boolean getValue() {
        return value != null;
    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        if (parts.size() == 1) {
            String name = parts.get(0).getName();
            if (name.equals("prefix!") || name.equals("not")) {
                return new GraceBoolean(!success);
            } else if (name.equals("&&")) {
                if (success) {
                    return parts.get(0).getArgs().get(0);
                }
                return new GraceBoolean(false);
            } else if (name.equals("||")) {
                if (success) {
                    return new GraceBoolean(true);
                }
                return parts.get(0).getArgs().get(0);
            } else if (name.equals("match")) {
                return this;
            } else if (name.equals("value")) {
                return value;
            } else if (name.equals("asString")) {
                return new GraceString(toString());
            } else if (name.equals("succeeded")) {
                return new GraceBoolean(success);
            } else if (name.equals("==")) {
                return new GraceBoolean(this == parts.get(0).getArgs().get(0));
            } else if (name.equals("!=")) {
                return new GraceBoolean(this != parts.get(0).getArgs().get(0));
            } else if (name.equals("ifTrue")) {
                if (success) {
                    parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
                }
                return GraceDone.done;
            } else if (name.equals("ifFalse")) {
                if (!success) {
                    parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
                }
                return GraceDone.done;
            } else if (name.equals("ifSuccess")) {
                if (success) {
                    GraceObject result = parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of(value)))));
                    return new GraceMatchResult(true, result);
                }
                return new GraceMatchResult(false, value);
            } else if (name.equals("chain")) {
                if (success) {
                    return this;
                }
                GraceObject next = parts.get(0).getArgs().get(0);
                return next.request(new Request(request.getVisitor(), List.of(new RequestPartR("match", List.of(value)))));
            }
        } else if (parts.size() == 2) {
            String name = request.getName();
            if (name.equals("ifTrue(1)ifFalse(1)")) {
                if (success) {
                    return parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
                }
                return parts.get(1).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of()))));
            } else if (name.equals("ifSuccess(1)otherwise(1)")) {
                GraceObject succcessBlock = parts.get(0).getArgs().get(0);
                GraceObject otherwisePattern = parts.get(1).getArgs().get(0);
                if (success) {
                    GraceObject result = parts.get(0).getArgs().get(0).request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of(value)))));
                    return new GraceMatchResult(true, result);
                }
                return otherwisePattern.request(new Request(request.getVisitor(), List.of(new RequestPartR("match", List.of(value)))));
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in MatchResult: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }
}
