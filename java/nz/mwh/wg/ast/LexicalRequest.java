package nz.mwh.wg.ast;

import java.util.List;
import java.util.ArrayList;

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

    public LexicalRequest inheriting() {
        List<Part> parts = new ArrayList<>(this.parts);
        parts.add(new Part("inherit", Cons.fromValue(new LexicalRequest(location, Cons.fromValue(new Part("inherit object", null))))));
        return new LexicalRequest(location, Cons.fromList(parts));
    }

    public List<? extends Part> getParts() {
        return parts;
    }


}