package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceBoolean implements GraceObject {
    public static final GraceBoolean TRUE = new GraceBoolean(true);
    public static final GraceBoolean FALSE = new GraceBoolean(false);

    private boolean value;

    public GraceBoolean(boolean value) {
        this.value = value;
    }

    public static GraceBoolean of(boolean value) {
        return value ? TRUE : FALSE;
    }

    public String toString() {
        return value ? "true" : "false";
    }


    public boolean getValue() {
        return value;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args) {
        switch (methodName) {
            case "asString":
                return new PendingStep(ctx, returnCont, new GraceString(toString()));
            case "not":
            case "prefix!":
                return returnCont.returning(ctx, GraceBoolean.of(!this.value));
            case "==(1)":
                GraceBoolean eqBool = assertBoolean(args.get(0));
                boolean eqResult = this.value == eqBool.value;
                return returnCont.returning(ctx, GraceBoolean.of(eqResult));
            case "!=(1)":
                GraceBoolean neqBool = assertBoolean(args.get(0));
                boolean neqResult = this.value == neqBool.value;
                return returnCont.returning(ctx, GraceBoolean.of(!neqResult));
            case "&&(1)":
                GraceBoolean andBool = assertBoolean(args.get(0));
                boolean andResult = this.value && andBool.value;
                return returnCont.returning(ctx, GraceBoolean.of(andResult));
            case "||(1)":
                GraceBoolean orBool = assertBoolean(args.get(0));
                boolean orResult = this.value || orBool.value;
                return returnCont.returning(ctx, GraceBoolean.of(orResult));
            default:
                System.out.println("no such method " + methodName + " on Boolean");
                return new PendingStep(ctx, returnCont, null);
        }
    }

    public static GraceBoolean assertBoolean(GraceObject obj) {
        if (obj instanceof GraceBoolean bool) {
            return bool;
        } else {
            throw new RuntimeException("Expected a Boolean, got: " + obj);
        }
    }
}
