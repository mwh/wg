package nz.mwh.wg.runtime;

import java.util.List;

public class GracePatternOr implements GraceObject {
    GraceObject left;
    GraceObject right;

    public GracePatternOr(GraceObject left, GraceObject right) {
        this.left = left;
        this.right = right;
    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        if (parts.size() == 1) {
            String name = parts.get(0).getName();
            if (name.equals("|")) {
                return new GracePatternOr(this, parts.get(0).getArgs().get(0));
            } else if (name.equals("match")) {
                GraceObject target = parts.get(0).getArgs().get(0);
                GraceObject leftResult = left.request(new Request(request.getVisitor(), List.of(new RequestPartR("match", List.of(target)))));

                GraceObject leftSuccess = leftResult.request(new Request(request.getVisitor(), List.of(new RequestPartR("succeeded", List.of()))));
                if (leftSuccess instanceof GraceBoolean leftBool && leftBool.getValue()) {
                    return leftResult;
                }
                
                GraceObject rightResult = right.request(new Request(request.getVisitor(), List.of(new RequestPartR("match", List.of(target)))));
                return rightResult;
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

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        if (left instanceof GraceTypeReference tr) {
            sb.append(tr.getName());
        } else {
            sb.append(left.toString());
        }
        sb.append(" | ");
        if (right instanceof GraceTypeReference tr) {
            sb.append(tr.getName());
        } else {
            sb.append(right.toString());
        }
        return sb.toString();
    }
}
