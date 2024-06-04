package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class NumberNode extends ASTNode {
    double value;

    public NumberNode(double value) {
        this.value = value;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "numberNode(" + value + ")";
    }

    public double getValue() {
        return value;
    }
}