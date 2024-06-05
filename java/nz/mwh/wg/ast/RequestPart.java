package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class RequestPart {
    String name;
    List<ASTNode> args;

    public RequestPart(String name, Cons<ASTNode> args) {
        this.name = name;
        this.args = args.toList();
    }
    
    <T> T accept(T context, Visitor<T> visitor) {
        throw new RuntimeException("Cannot visit RequestPart");
    }

    public String toString() {
        return "requestPart(\"" + name + "\", " + Cons.stringFromList(args) + ")";
    }

    public String getName() {
        return name;
    }

    public List<ASTNode> getArgs() {
        return args;
    }

}