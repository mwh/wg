package nz.mwh.wg.ast.grace;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.runtime.GraceObject;
import nz.mwh.wg.runtime.Request;

public class Block extends nz.mwh.wg.ast.Block implements GraceObject {
    
    public Block(Cons<ASTNode> parameters, Cons<ASTNode> body) {
        super(parameters, body);
    }

    public Block(Cons<ASTNode> parameters, Cons<ASTNode> body, String startPos, String endPos) {
        super(parameters, body, startPos, endPos);
    }

    public GraceObject request(Request request) {
        throw new RuntimeException("Cannot request method on ASTNode: " + request.getName());
    }

    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }

}
