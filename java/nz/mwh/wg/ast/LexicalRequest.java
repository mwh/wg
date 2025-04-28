package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class LexicalRequest extends ASTNode {
    List<? extends Part> parts;

    public LexicalRequest(String location, Cons<? extends Part> parts) {
        this.parts = parts.toList();
        this.location = location;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "lexReq(" + Cons.stringFromList(parts) + ")";
    }

    public List<? extends Part> getParts() {
        return parts;
    }


}