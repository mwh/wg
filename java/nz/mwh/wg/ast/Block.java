package nz.mwh.wg.ast;

import java.util.List;

import nz.mwh.wg.Visitor;

public class Block extends ASTNode {
    List<ASTNode> parameters;
    List<ASTNode> body;
    String startPos;
    String endPos;

    public Block(Cons<ASTNode> parameters, Cons<ASTNode> body) {
        this(parameters, body, null, null);
    }

    public Block(Cons<ASTNode> parameters, Cons<ASTNode> body, String startPos, String endPos) {
        this.parameters = parameters.toList();
        this.body = body.toList();
        this.startPos = startPos;
        this.endPos = endPos;
    }

    
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    public String toString() {
        return "block(" + Cons.stringFromList(parameters) + ", " + Cons.stringFromList(body) + ")";
    }

    public List<ASTNode> getParameters() {
        return parameters;
    }

    public List<ASTNode> getBody() {
        return body;
    }
}
