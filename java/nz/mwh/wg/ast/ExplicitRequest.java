package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class ExplicitRequest extends ASTNode {
    ASTNode receiver;
    List<? extends RequestPart> parts;
    public String location;

    public ExplicitRequest(String location, ASTNode receiver, Cons<? extends RequestPart> parts) {
        this.receiver = receiver;
        this.parts = parts.toList();
        this.location = location;
    }
    
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "explicitRequest(" + receiver + ", " + Cons.stringFromList(parts) + ")";
    }

    public ASTNode getReceiver() {
        return receiver;
    }

    public List<? extends RequestPart> getParts() {
        return parts;
    }
}
