package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;


public class MethodSignature extends ASTNode {
    List<? extends Part> parts;
    ASTNode returnType;

    public MethodSignature(Cons<? extends Part> parts, ASTNode type) {
        this.parts = parts.toList();
        this.returnType = type;
    }

    public <T> T accept(T context, Visitor<T> visitor) {
        throw new UnsupportedOperationException("Unimplemented method 'accept'");
    }

    public String toString() {
        return "methSig(" + Cons.stringFromList(parts) + ", " + (returnType == null ? "nil" : "one(" + returnType + ")") + ")";
    }

    public List<? extends Part> getParts() {
        return parts;
    }

    public ASTNode getReturnType() {
        return returnType;
    }
}
