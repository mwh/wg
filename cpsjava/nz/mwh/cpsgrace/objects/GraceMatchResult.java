package nz.mwh.cpsgrace.objects;

import java.util.List;
import java.util.Set;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

/**
 * Runtime representation of a Grace match result.
 * Encapsulates whether a pattern match succeeded, and the matched value.
 */
public class GraceMatchResult implements GraceObject {
    private final boolean succeeded;
    private final GraceObject value;

    public GraceMatchResult(boolean succeeded, GraceObject value) {
        this.succeeded = succeeded;
        this.value = value;
    }

    public boolean isSuccess() {
        return succeeded;
    }

    @Override
    public String toString() {
        return succeeded ? "MatchResult(succeeded with " + value + ")" : "MatchResult(failed)";
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "succeeded":
                return returnCont.returning(ctx, GraceBoolean.of(succeeded));

            case "value":
                return returnCont.returning(ctx, value);

            case "ifTrue(1)": {
                GraceObject block = args.get(0);
                if (succeeded) {
                    return block.requestMethod(ctx, returnCont, "apply", List.of());
                }
                return returnCont.returning(ctx, GraceObject.DONE);
            }

            case "ifFalse(1)": {
                GraceObject block = args.get(0);
                if (!succeeded) {
                    return block.requestMethod(ctx, returnCont, "apply", List.of());
                }
                return returnCont.returning(ctx, GraceObject.DONE);
            }

            case "ifTrue(1)ifFalse(1)": {
                GraceObject trueBlock = args.get(0);
                GraceObject falseBlock = args.get(1);
                if (succeeded) {
                    return trueBlock.requestMethod(ctx, returnCont, "apply", List.of());
                }
                return falseBlock.requestMethod(ctx, returnCont, "apply", List.of());
            }

            case "ifSuccess(1)": {
                // Apply block with the matched value if succeeded
                GraceObject block = args.get(0);
                if (succeeded) {
                    return block.requestMethod(ctx, returnCont, "apply(1)", List.of(value));
                }
                return returnCont.returning(ctx, GraceObject.DONE);
            }

            case "ifSuccess(1)otherwise(1)": {
                GraceObject successBlock = args.get(0);
                GraceObject otherwiseBlock = args.get(1);
                if (succeeded) {
                    return successBlock.requestMethod(ctx, returnCont, "apply(1)", List.of(value));
                }
                return otherwiseBlock.requestMethod(ctx, returnCont, "apply", List.of());
            }

            case "chain(1)": {
                // If succeeded, run the next pattern on the same value
                GraceObject nextPattern = args.get(0);
                if (succeeded) {
                    return nextPattern.requestMethod(ctx, returnCont, "match(1)", List.of(value));
                }
                return returnCont.returning(ctx, this);
            }

            case "match(1)":
                // A match result treated as a pattern always succeeds/fails the same way
                return returnCont.returning(ctx, this);

            case "prefix!":
                return returnCont.returning(ctx, new GraceMatchResult(!succeeded, value));

            case "&&(1)": {
                GraceMatchResult other = assertMatchResult(args.get(0));
                return returnCont.returning(ctx, new GraceMatchResult(succeeded && other.succeeded, value));
            }

            case "||(1)": {
                GraceMatchResult other = assertMatchResult(args.get(0));
                return returnCont.returning(ctx, new GraceMatchResult(succeeded || other.succeeded, value));
            }

            case "==(1)": {
                GraceMatchResult other = assertMatchResult(args.get(0));
                return returnCont.returning(ctx, GraceBoolean.of(succeeded == other.succeeded));
            }

            case "!=(1)": {
                GraceMatchResult other = assertMatchResult(args.get(0));
                return returnCont.returning(ctx, GraceBoolean.of(succeeded != other.succeeded));
            }

            case "asString":
                return returnCont.returning(ctx, new GraceString(toString()));

            default:
                throw new RuntimeException("No such method on MatchResult: " + methodName);
        }
    }

    private static Set<String> METHODS = Set.of(
        "succeeded", "value", "ifTrue(1)", "ifFalse(1)", "ifTrue(1)ifFalse(1)",
        "ifSuccess(1)", "ifSuccess(1)otherwise(1)", "chain(1)", "match(1)",
        "prefix!", "&&(1)", "||(1)", "==(1)", "!=(1)", "asString"
    );

    @Override
    public boolean hasMethod(String name) {
        return METHODS.contains(name);
    }

    public static GraceMatchResult assertMatchResult(GraceObject obj) {
        if (obj instanceof GraceMatchResult mr) {
            return mr;
        }
        throw new RuntimeException("Expected a MatchResult, got: " + obj);
    }
}
