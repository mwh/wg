package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class ImportStmt extends ASTNode {
    String source;
    IdentifierDeclaration binding;
    String pos;

    public ImportStmt(String src, IdentifierDeclaration bnd) {
        this(null, src, bnd);
    }

    public ImportStmt(String pos, String src, IdentifierDeclaration bnd) {
        this.pos = pos;
        this.source = src;
        this.binding = bnd;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "importStmt(\"" + source + "\", " + binding + ")";
    }

    public String getName() {
        return binding.getName();
    }

    public IdentifierDeclaration getBinding() {
        return binding;
    }

    public String getSource() {
        return source;
    }
}