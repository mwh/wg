package nz.mwh.wg.ast.grace;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.runtime.GraceObject;
import nz.mwh.wg.runtime.Request;

public class Lineup extends nz.mwh.wg.ast.Lineup implements GraceObject {

    public Lineup(Cons<ASTNode> elements) {
        super(elements);
    }
    
    public GraceObject request(Request request) {
        throw new RuntimeException("Cannot request method on ASTNode: " + request.getName());
    }

    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }

}
