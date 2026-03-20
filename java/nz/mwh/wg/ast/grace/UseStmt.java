package nz.mwh.wg.ast.grace;

import nz.mwh.wg.runtime.GraceObject;
import nz.mwh.wg.runtime.Request;
import nz.mwh.wg.ast.ASTNode;

public class UseStmt extends nz.mwh.wg.ast.UseStmt implements GraceObject {
    
    public UseStmt(ASTNode parent, Cons<ASTNode> extra) {
        super(parent, extra);
    }

    public GraceObject request(Request request) {
        throw new RuntimeException("Cannot request method on ASTNode: " + request.getName());
    }

    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }

}
