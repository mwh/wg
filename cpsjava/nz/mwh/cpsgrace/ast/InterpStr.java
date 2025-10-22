package nz.mwh.cpsgrace.ast;

import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.objects.GraceString;

public class InterpStr extends ASTNode {
    private String prefix;
    private ASTNode expression;
    private ASTNode next;

    public InterpStr(String prefix, ASTNode expression, ASTNode next) {
        this.prefix = prefix;
        this.expression = expression;
        this.next = next;
    }

    public ASTNode getExpression() {
        return expression;
    }

    public ASTNode getNext() {
        return next;
    }

    public String getPrefix() {
        return prefix;
    }

    public CPS toCPS() {
        CPS exprCPS = expression.toCPS();
        CPS nextCPS = next.toCPS();
        return (ctx, cont) -> {
            return exprCPS.run(ctx, (exprValue) -> {
                return exprValue.requestMethod(ctx, (exprString) -> {
                    String mid = GraceString.assertString(exprString).toString();
                    return nextCPS.run(ctx, (nextValue) -> {
                        String combined = prefix + mid + GraceString.assertString(nextValue).toString();
                        return cont.apply(new nz.mwh.cpsgrace.objects.GraceString(combined));
                    });
                }, "asString", List.of());
            });
        };
    }
    
}
