package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class LexicalRequest extends ASTNode {
    Cons<? extends RequestPart> parts;

    public LexicalRequest(Cons<? extends RequestPart> parts) {
        this.parts = parts;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "lexicalRequest(" + parts + ")";
    }

    public Cons<? extends RequestPart> getParts() {
        return parts;
    }
}