package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class RequestPart {
    String name;
    Cons<ASTNode> args;

    public RequestPart(String name, Cons<ASTNode> args) {
        this.name = name;
        this.args = args;
    }
    
    <T> T accept(T context, Visitor<T> visitor) {
        throw new RuntimeException("Cannot visit RequestPart");
    }

    public String toString() {
        return "requestPart(" + name + ", " + args + ")";
    }

    public String getName() {
        return name;
    }

    public Cons<ASTNode> getArgs() {
        return args;
    }

}