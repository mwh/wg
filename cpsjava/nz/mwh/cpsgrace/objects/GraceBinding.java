package nz.mwh.cpsgrace.objects;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceBinding implements GraceObject {
    private GraceObject key;
    private GraceObject value;

    public GraceBinding(GraceObject key, GraceObject value) {
        this.key = key;
        this.value = value;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "key":
                return returnCont.returning(ctx, key);
            case "value":
                return returnCont.returning(ctx, value);
            case "==(1)":
                GraceObject other = args.get(0);
                if (other instanceof GraceBinding otherBinding) {
                    return key.requestMethod(ctx, (GraceObject keyEq) -> {
                        return value.requestMethod(ctx, (GraceObject valueEq) -> {
                            return keyEq.requestMethod(ctx, (GraceObject bothEq) -> {
                                returnCont.returning(ctx, bothEq);
                                return null;
                            }, "&&", List.of(valueEq));
                        }, "==", List.of(otherBinding.value));
                    }, "==", List.of(otherBinding.key));
                } else {
                    return returnCont.returning(ctx, GraceBoolean.FALSE);
                }
            case "!=(1)":
                return requestMethod(ctx, (GraceObject eq) -> {
                    return eq.requestMethod(ctx, (GraceObject notEq) -> {
                        returnCont.returning(ctx, notEq);
                        return null;
                    }, "prefix!", List.of());
                }, "==", List.of(args.get(0)));
            case "::(1)":
                return returnCont.returning(ctx, new GraceBinding(this, args.get(0)));
            case "asString": {
                return key.requestMethod(ctx, (GraceObject keyStr) -> {
                    return value.requestMethod(ctx, (GraceObject valueStr) -> {
                        returnCont.returning(ctx, new GraceString(keyStr.toString() + "::" + valueStr.toString()));
                        return null;
                    }, "asDebugString", List.of());
                }, "asDebugString", List.of());
            }
            case "asDebugString": {
                return key.requestMethod(ctx, (GraceObject keyStr) -> {
                    return value.requestMethod(ctx, (GraceObject valueStr) -> {
                        returnCont.returning(ctx, new GraceString("Binding(" + keyStr.toString() + ", " + valueStr.toString() + ")"));
                        return null;
                    }, "asDebugString", List.of());
                }, "asDebugString", List.of());
            }
            case "hash": {
                return key.requestMethod(ctx, (GraceObject keyHash) -> {
                    return value.requestMethod(ctx, (GraceObject valueHash) -> {
                        int combinedHash = ((GraceNumber) keyHash).intValue() ^ ((GraceNumber) valueHash).intValue() ^ 0xB14D1465;
                        returnCont.returning(ctx, new GraceNumber(Integer.toUnsignedLong(combinedHash)));
                        return null;
                    }, "hash", List.of());
                }, "hash", List.of());
            }
            default:
                throw new RuntimeException("No such method " + methodName + " on primitive array");
        }
    }

    @Override
    public boolean hasMethod(String name) {
        return switch (name) {
            case "key", "value", "==(1)", "!=(1)", "::(1)", "asString", "asDebugString", "hash" -> true;
            default -> false;
        };
    }
}
