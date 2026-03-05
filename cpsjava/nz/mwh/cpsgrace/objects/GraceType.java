package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.ast.MethSig;

public class GraceType implements GraceObject {
    private final List<MethSig> signatures;

    public GraceType(List<MethSig> signatures) {
        this.signatures = signatures;
    }

    public List<MethSig> getSignatures() {
        return signatures;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args, List<GraceObject> genericArgs) {
        switch (methodName) {
            case "match(1)": {
                GraceObject target = args.get(0);
                boolean allMatch = signatures.stream().allMatch(sig -> target.hasMethod(sig.getName()));
                return returnCont.returning(ctx, new GraceMatchResult(allMatch, target));
            }
            case "|(1)":
                return returnCont.returning(ctx, new GracePatternOr(this, args.get(0)));
            case "asString":
                return returnCont.returning(ctx, new GraceString(toString()));
            default:
                throw new RuntimeException("Cannot request method on type object: " + methodName);
        }
    }

    @Override
    public String toString() {
        return "<type>";
    }
}
