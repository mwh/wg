package nz.mwh.cpsgrace.ast;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.GraceString;

public class StrLit extends ASTNode {
    private String value;

    public StrLit(String value) {
        this.value = value;
    }

    public String getValue() {
        return value;
    }

    public CPS toCPS() {
        return (ctx, cont) -> new PendingStep(ctx, cont, new GraceString(value));
    }

}
