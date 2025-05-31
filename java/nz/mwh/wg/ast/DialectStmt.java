package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class DialectStmt extends ASTNode {
    String source;

    public DialectStmt(String value) {
        this.source = value;
    }
    
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "dialectStmt(\"" + source + "\")";
    }

    public String getSource() {
        return source;
    }
}
