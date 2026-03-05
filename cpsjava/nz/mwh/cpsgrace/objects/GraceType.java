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
        throw new RuntimeException("Cannot request method on type object: " + methodName);
    }

    @Override
    public String toString() {
        return "<type>";
    }
}
