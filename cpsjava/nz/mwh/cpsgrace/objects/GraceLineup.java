package nz.mwh.cpsgrace.objects;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceLineup implements GraceObject {
    private List<GraceObject> elements;

    public GraceLineup(List<GraceObject> elements) {
        this.elements = elements;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "do(1)":
            case "each(1)": {
                GraceObject block = args.get(0);
                return doHelper(ctx, returnCont, block, 0);
            }
            case "map(1)": {
                GraceObject block = args.get(0);
                List<GraceObject> results = new ArrayList<>();
                return mapHelper(ctx, returnCont, block, 0, results);
            }
            case "size": {
                return returnCont.returning(ctx, new GraceNumber(elements.size()));
            }
            case "at(1)": {
                GraceNumber index = GraceNumber.assertNumber(args.get(0));
                return returnCont.returning(ctx, elements.get(index.intValue() - 1));
            }
            case "++(1)": {
                GraceObject other = args.get(0);
                if (other instanceof GraceLineup otherLineup) {
                    List<GraceObject> combined = new ArrayList<>(elements);
                    combined.addAll(otherLineup.elements);
                    return returnCont.returning(ctx, new GraceLineup(combined));
                }
                throw new RuntimeException("Cannot concatenate lineup with " + other);
            }
            case "asString": {
                return asStringHelper(ctx, returnCont, 0, new StringBuilder("["));
            }
            case "asDebugString": {
                return asStringHelper(ctx, returnCont, 0, new StringBuilder("["));
            }
            default:
                throw new RuntimeException("No such method " + methodName + " on Lineup");
        }
    }

    private PendingStep doHelper(Context ctx, Continuation returnCont, GraceObject block, int index) {
        if (index >= elements.size()) {
            return returnCont.returning(ctx, GraceObject.DONE);
        }
        return block.requestMethod(ctx, (_) -> doHelper(ctx, returnCont, block, index + 1),
            "apply(1)", List.of(elements.get(index)));
    }

    private PendingStep mapHelper(Context ctx, Continuation returnCont, GraceObject block, int index, List<GraceObject> results) {
        if (index >= elements.size()) {
            return returnCont.returning(ctx, new GraceLineup(results));
        }
        return block.requestMethod(ctx, (GraceObject result) -> {
            results.add(result);
            return mapHelper(ctx, returnCont, block, index + 1, results);
        }, "apply(1)", List.of(elements.get(index)));
    }

    private PendingStep asStringHelper(Context ctx, Continuation returnCont, int index, StringBuilder sb) {
        if (index >= elements.size()) {
            sb.append("]");
            return returnCont.returning(ctx, new GraceString(sb.toString()));
        }
        if (index > 0) {
            sb.append(", ");
        }
        return elements.get(index).requestMethod(ctx, (GraceObject strObj) -> {
            sb.append(strObj.toString());
            return asStringHelper(ctx, returnCont, index + 1, sb);
        }, "asDebugString", List.of());
    }

    @Override
    public boolean hasMethod(String name) {
        return switch (name) {
            case "each(1)", "do(1)", "map(1)", "size", "at(1)", "++(1)", "asString" -> true;
            default -> false;
        };
    }
}
