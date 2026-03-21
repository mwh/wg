package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceExceptionKind implements GraceObject {
    private GraceExceptionKind parent;
    private String label;

    public GraceExceptionKind(GraceExceptionKind parent, String label) {
        this.parent = parent;
        this.label = label;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "refine(1)":
                return returnCont.returning(ctx, new GraceExceptionKind(this, args.get(0).toString()));
            case "asString":
                return returnCont.returning(ctx, new GraceString(label));
            case "asDebugString":
                return returnCont.returning(ctx, new GraceString("ExceptionKind(" + label + (parent != null ? ", parent=" + parent.label : "") + ")"));
            case "raise(1)":
                GraceObject messageObj = args.get(0);
                return messageObj.requestMethod(ctx, (GraceObject msgStr) -> {
                    UserObject exnValue = new UserObject();
                    exnValue.setDebugLabel("exception: " + label);
                    exnValue.addMethod("asString", Method.java((c, k, _, _) -> k.returning(c, msgStr)));
                    exnValue.addMethod("message", Method.java((c, k, _, _) -> k.returning(c, msgStr)));
                    exnValue.addMethod("name", Method.java((c, k, _, _) -> k.returning(c, new GraceString(label))));
                    exnValue.addMethod("asString", Method.java((c, k, _, _) -> k.returning(c, new GraceString(label + ": " + msgStr))));
                    exnValue.addMethod("reraise", Method.java((c, k, _, _) -> {
                        Continuation exnK = c.getExceptionContinuation();
                        if (exnK != null) {
                            return exnK.apply(exnValue);
                        }
                        throw new RuntimeException("Unhandled exception: " + label + ": " + msgStr);
                    }));
                    Continuation exnK = ctx.getExceptionContinuation();
                    if (exnK != null) {
                        return exnK.apply(exnValue);
                    }
                    throw new RuntimeException("Unhandled exception: " + label + ": " + msgStr);
                }, "asString", java.util.List.of());

            case "match(1)":
                // For now, exception types always match
                GraceObject target = args.get(0);
                return returnCont.returning(ctx, new GraceMatchResult(true, target));
            default:
                throw new RuntimeException("No such method " + methodName + " on exception kind");
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
