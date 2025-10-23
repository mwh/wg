package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class GraceRange implements GraceObject {
    private final double start;
    private final double end;
    private final double step;

    public GraceRange(double start, double end, double step) {
        this.start = start;
        this.end = end;
        this.step = step;
    }

    public GraceRange(double start, double end) {
        this(start, end, 1.0);
    }

    public double getStart() {
        return start;
    }

    public double getEnd() {
        return end;
    }

    public double getStep() {
        return step;
    }

    private PendingStep each(Context ctx, Continuation returnCont, GraceObject funcObj, double current) {
        Context currentCtx = ctx;

        if (step > 0 && current > end || step < 0 && current < end) {
            return returnCont.returning(currentCtx, GraceObject.DONE);
        }
        return new PendingStep(ctx, (_) -> {
            return funcObj.requestMethod(ctx, _ ->  each(currentCtx, returnCont, funcObj, current + step),
                "apply(1)", List.of(new GraceNumber(current)));
        }, null);
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args) {
        switch (methodName) {
            case "asString":
                String rangeStr = "Range(" + start + " .. " + end + " by " + step + ")";
                return new PendingStep(ctx, returnCont, new GraceString(rangeStr));
            case "each(1)":
                GraceObject funcObj = args.get(0);
                return each(ctx, returnCont, funcObj, start);
            case "..(1)":
                GraceNumber stepNum = GraceNumber.assertNumber(args.get(0));
                GraceRange range = new GraceRange(this.start, this.end, stepNum.doubleValue());
                return returnCont.returning(ctx, range);
            default:
                System.out.println("no such method " + methodName + " on Range");
                return new PendingStep(ctx, returnCont, null);
        }
    }
    
}
