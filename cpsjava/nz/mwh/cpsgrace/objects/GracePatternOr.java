package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

/**
 * A pattern that succeeds if either of two sub-patterns matches.
 * Created by the |(1) method on any pattern object.
 */
public class GracePatternOr implements GraceObject {
    private final GraceObject left;
    private final GraceObject right;

    public GracePatternOr(GraceObject left, GraceObject right) {
        this.left = left;
        this.right = right;
    }

    @Override
    public String toString() {
        return left + " | " + right;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "match(1)": {
                GraceObject target = args.get(0);
                // Try the left pattern first; if it succeeds return it, otherwise try right
                return left.requestMethod(ctx, (GraceObject leftResult) -> {
                    GraceMatchResult mr = GraceMatchResult.assertMatchResult(leftResult);
                    if (mr.isSuccess()) {
                        return returnCont.returning(ctx, mr);
                    }
                    return right.requestMethod(ctx, returnCont, "match(1)", List.of(target));
                }, "match(1)", List.of(target));
            }

            case "|(1)": {
                GraceObject other = args.get(0);
                return returnCont.returning(ctx, new GracePatternOr(this, other));
            }

            case "asString":
                return returnCont.returning(ctx, new GraceString(toString()));

            default:
                throw new RuntimeException("No such method on PatternOr: " + methodName);
        }
    }
}
