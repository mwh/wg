package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class ExplicitRequest extends ASTNode {
    ASTNode receiver;
    Cons<? extends RequestPart> parts;
    public String location;

    public ExplicitRequest(String location, ASTNode receiver, Cons<? extends RequestPart> parts) {
        this.receiver = receiver;
        this.parts = parts;
        this.location = location;
    }
    
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "explicitRequest(" + receiver + ", " + parts + ")";
    }

    public ASTNode getReceiver() {
        return receiver;
    }

    public Cons<? extends RequestPart> getParts() {
        return parts;
    }
}
