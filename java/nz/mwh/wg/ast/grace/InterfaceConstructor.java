package nz.mwh.wg.ast.grace;

import nz.mwh.wg.Visitor;
import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.runtime.GraceObject;
import nz.mwh.wg.runtime.Request;

public class InterfaceConstructor extends nz.mwh.wg.ast.InterfaceConstructor implements GraceObject {

    public InterfaceConstructor(java.util.List<ASTNode> body) {
        super(body);
    }

    public InterfaceConstructor(Cons<ASTNode> body) {
        super(body.toList());
    }

    @Override
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

    @Override
    public String toString() {
        return "interfaceCons(" + Cons.stringFromList(getBody()) + ")";
    }
    
    public GraceObject request(Request request) {
        throw new RuntimeException("Cannot request method on ASTNode: " + request.getName());
    }

    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
    
}
