package nz.mwh.cpsgrace.ast;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.GraceNumber;

public class NumLit extends ASTNode {
    private Number value;

    public NumLit(Number value) {
        this.value = value;
    }

    public Number getValue() {
        return value;
    }

    public CPS toCPS() {
        return (ctx, cont) -> new PendingStep(ctx, cont, new GraceNumber(value));
    }

}
