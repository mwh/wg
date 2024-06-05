package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class StringNode extends ASTNode {
    String value;

    public StringNode(String value) {
        this.value = value;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "stringNode(\"" +
            value.replace("\\", "\\\\")
                .replace("\n", "\\n")
                .replace("\r", "\\r")
                .replace("\"", "\\\"")
            + "\")";
    }

    public String getValue() {
        return value;
    }
}
