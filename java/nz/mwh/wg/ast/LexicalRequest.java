package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class LexicalRequest extends ASTNode {
    List<? extends RequestPart> parts;

    public LexicalRequest(Cons<? extends RequestPart> parts) {
        this.parts = parts.toList();
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "lexicalRequest(" + Cons.stringFromList(parts) + ")";
    }

    public List<? extends RequestPart> getParts() {
        return parts;
    }
}