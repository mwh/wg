package nz.mwh.cpsgrace.objects;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GracePrimitiveArray implements GraceObject {
    private GraceObject[] elements;

    public GracePrimitiveArray(int size) {
        this.elements = new GraceObject[size];
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "do(1)":
            case "each(1)": {
                GraceObject block = args.get(0);
                return doHelper(ctx, returnCont, block, 0);
            }
            case "size": {
                return returnCont.returning(ctx, new GraceNumber(elements.length));
            }
            case "at(1)": {
                GraceNumber index = GraceNumber.assertNumber(args.get(0));
                return returnCont.returning(ctx, elements[index.intValue()]);
            }
            case "at(1)put(1)": {
                GraceNumber index = GraceNumber.assertNumber(args.get(0));
                elements[index.intValue()] = args.get(1);
                return returnCont.returning(ctx, GraceObject.DONE);
            }
            case "asString": {
                return asStringHelper(ctx, returnCont, 0, new StringBuilder("primitiveArray.new["));
            }
            default:
                throw new RuntimeException("No such method " + methodName + " on primitive array");
        }
    }

    private PendingStep doHelper(Context ctx, Continuation returnCont, GraceObject block, int index) {
        if (index >= elements.length) {
            return returnCont.returning(ctx, GraceObject.DONE);
        }
        return block.requestMethod(ctx, (_) -> doHelper(ctx, returnCont, block, index + 1),
            "apply(1)", List.of(elements[index]));
    }

    private PendingStep asStringHelper(Context ctx, Continuation returnCont, int index, StringBuilder sb) {
        if (index >= elements.length) {
             sb.append("]");
             return returnCont.returning(ctx, new GraceString(sb.toString()));
        }
        if (index > 0) {
            sb.append(", ");
        }
        return elements[index].requestMethod(ctx, (GraceObject strObj) -> {
            sb.append(strObj.toString());
            return asStringHelper(ctx, returnCont, index + 1, sb);
        }, "asString", List.of());
    }

    @Override
    public boolean hasMethod(String name) {
        return switch (name) {
            case "do(1)", "size", "at(1)", "at(1)put(1)", "asString" -> true;
            default -> false;
        };
    }
}
