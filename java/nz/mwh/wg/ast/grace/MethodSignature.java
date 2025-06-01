package nz.mwh.wg.ast.grace;
import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Cons;
import nz.mwh.wg.ast.Part;
import nz.mwh.wg.runtime.GraceObject;
import nz.mwh.wg.runtime.Request;

public class MethodSignature extends nz.mwh.wg.ast.MethodSignature implements GraceObject {

    public MethodSignature(Cons<? extends Part> parts, ASTNode type) {
        super(parts, type);
    }

    @Override
    public GraceObject request(Request request) {
        throw new RuntimeException("Cannot request method on ASTNode: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}