package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceNumber implements GraceObject {
    private Number value;

    public GraceNumber(Number val) {
        this.value = val;
    }
    
    public String toString() {
        return value.toString();
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args) {
        switch (methodName) {
            case "asString":
                return new PendingStep(ctx, returnCont, new GraceString(toString()));
            case "+(1)":
                GraceNumber arg0 = assertNumber(args.get(0));
                Number sum = add(this.value, arg0.value);
                return returnCont.returning(ctx, new GraceNumber(sum));
            case "-(1)":
                GraceNumber arg1 = assertNumber(args.get(0));
                Number diff = subtract(this.value, arg1.value);
                return returnCont.returning(ctx, new GraceNumber(diff));
            case "*(1)":
                GraceNumber arg2 = assertNumber(args.get(0));
                Number prod = multiply(this.value, arg2.value);
                return returnCont.returning(ctx, new GraceNumber(prod));
            case "/(1)":
                GraceNumber arg3 = assertNumber(args.get(0));
                Number quot = divide(this.value, arg3.value);
                return returnCont.returning(ctx, new GraceNumber(quot));
            case "==(1)":
                GraceNumber eqNum = assertNumber(args.get(0));
                boolean eqResult = this.value.equals(eqNum.value);
                return returnCont.returning(ctx, GraceBoolean.of(eqResult));
            case "!=(1)":
                GraceNumber neqNum = assertNumber(args.get(0));
                boolean neqResult = this.value.equals(neqNum.value);
                return returnCont.returning(ctx, GraceBoolean.of(!neqResult));
            case "<(1)":
                GraceNumber ltNum = assertNumber(args.get(0));
                boolean ltResult = this.value.doubleValue() < ltNum.value.doubleValue();
                return returnCont.returning(ctx, GraceBoolean.of(ltResult));
            case ">(1)":
                GraceNumber gtNum = assertNumber(args.get(0));
                boolean gtResult = this.value.doubleValue() > gtNum.value.doubleValue();
                return returnCont.returning(ctx, GraceBoolean.of(gtResult));
            case ">=(1)":
                GraceNumber gteNum = assertNumber(args.get(0));
                boolean gteResult = this.value.doubleValue() >= gteNum.value.doubleValue();
                return returnCont.returning(ctx, GraceBoolean.of(gteResult));
            case "<=(1)":
                GraceNumber lteNum = assertNumber(args.get(0));
                boolean lteResult = this.value.doubleValue() <= lteNum.value.doubleValue();
                return returnCont.returning(ctx, GraceBoolean.of(lteResult));
            case "prefix-":
                Number neg = multiply(this.value, -1);
                return returnCont.returning(ctx, new GraceNumber(neg));
            default:
                System.out.println("no such method " + methodName + " on Number");
                return new PendingStep(ctx, returnCont, null);
        }
    }

    public int intValue() {
        return value.intValue();
    }

    private Number add(Number x, Number y) {
        if (x instanceof Integer && y instanceof Integer) {
            return x.intValue() + y.intValue();
        } else {
            return x.doubleValue() + y.doubleValue();
        }
    }

    private Number subtract(Number x, Number y) {
        if (x instanceof Integer && y instanceof Integer) {
            return x.intValue() - y.intValue();
        } else {
            return x.doubleValue() - y.doubleValue();
        }
    }

    private Number multiply(Number x, Number y) {
        if (x instanceof Integer && y instanceof Integer) {
            return x.intValue() * y.intValue();
        } else {
            return x.doubleValue() * y.doubleValue();
        }
    }

    private Number divide(Number x, Number y) {
        double result = x.doubleValue() / y.doubleValue();
        if (result == Math.floor(result)) {
            return (int) result;
        } else {
            return result;
        }
    }

    public static GraceNumber assertNumber(GraceObject obj) {
        if (obj instanceof GraceNumber gn) {
            return gn;
        } else {
            throw new RuntimeException("Expected a Number, got: " + obj);
        }
    }
}
