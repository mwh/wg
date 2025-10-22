package nz.mwh.cpsgrace.ast;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;

public class ReturnStmt extends ASTNode {
    private ASTNode expr;

    public ReturnStmt(ASTNode expr) {
        this.expr = expr;
    }

    public ASTNode getExpr() {
        return expr;
    }
    
    public CPS toCPS() {
        if (expr == null) {
            return (ctx, _) -> ctx.getReturnContinuation().apply(GraceObject.DONE);
        }
        CPS exprCPS = expr.toCPS();
        return (ctx, _) -> exprCPS.run(ctx, value -> ctx.getReturnContinuation().apply(value));
    }
}
