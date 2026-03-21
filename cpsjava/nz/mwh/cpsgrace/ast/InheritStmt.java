package nz.mwh.cpsgrace.ast;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;

public class InheritStmt extends ASTNode {
    private ASTNode expression;

    public InheritStmt(ASTNode expression) {
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
