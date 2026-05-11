package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class DialectStmt extends ASTNode {
    String source;
    String pos;

    public DialectStmt(String value) {
        this(null, value);
    }

    public DialectStmt(String pos, String value) {
        this.pos = pos;
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
