package nz.mwh.cpsgrace.ast;

import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class VarDec extends ASTNode {
    private String name;
    private ASTNode declaredType;
    private List<ASTNode> annotations;
    private ASTNode valueExpr;

    public VarDec(String name, ASTNode declaredType, List<ASTNode> annotations, ASTNode valueExpr) {
        this.name = name;
        this.declaredType = declaredType;
        this.annotations = annotations;
        this.valueExpr = valueExpr;
    }

    public CPS toCPS() {
        // At this point, only evaluate the initialiser and invoke the := method (if applicable)
        if (valueExpr == null) {
            return (ctx, cont) -> new PendingStep(ctx, cont, null);
        }
        CPS valueCPS = valueExpr.toCPS();
        return (ctx, cont) -> {
            return valueCPS.run(ctx, (value) -> {
                GraceObject receiver = ctx.findReceiver(name + ":=(1)");
                return receiver.requestMethod(ctx, cont, name + ":=(1)", List.of(value));
            });
        };
    }

    public String getName() {
        return name;
    }

    public ASTNode getDeclaredType() {
        return declaredType;
    }

    public List<ASTNode> getAnnotations() {
        return annotations;
    }

    public ASTNode getValueExpr() {
        return valueExpr;
    }
}
