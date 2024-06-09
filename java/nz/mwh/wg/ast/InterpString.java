package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class InterpString extends ASTNode {

    private String value;
    private ASTNode expression;
    private ASTNode next;

    public InterpString(String value, ASTNode expr, ASTNode next) {
        this.value = value;
        this.expression = expr;
        this.next = next;
    }

    public String toString() {
        return "interpStr(\"" +
            value.replace("\\", "\\\\")
                .replace("\n", "\\n")
                .replace("\r", "\\r")
                .replace("\"", "\\\"")
            + "\", " + expression.toString() + ", " + next.toString() + ")";
    }

    public String getValue() {
        return value;
    }

    public ASTNode getExpression() {
        return expression;
    }

    public ASTNode getNext() {
        return next;
    }

    @Override
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }
    
}
