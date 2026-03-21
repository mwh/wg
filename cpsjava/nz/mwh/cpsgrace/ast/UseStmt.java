package nz.mwh.cpsgrace.ast;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;

public class UseStmt extends ASTNode {
    private ASTNode expression;

    public UseStmt(ASTNode expression) {
        this.expression = expression;
    }

    public ASTNode getExpression() {
        return expression;
    }

    public CPS toCPS() {
        // Handled specially in ObjCons; should not be evaluated directly.
        return (ctx, cont) -> cont.apply(GraceObject.DONE);
    }
}
