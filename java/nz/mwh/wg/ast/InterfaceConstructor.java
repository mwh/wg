package nz.mwh.wg.ast;

import java.util.Collections;
import java.util.List;

import nz.mwh.wg.Visitor;

public class InterfaceConstructor extends ASTNode {
    private List<MethodSignature> body;
    
    public InterfaceConstructor(List<ASTNode> body) {
        this.body = body.stream().map(x -> (MethodSignature) x).toList();
    }

    public InterfaceConstructor(Cons<ASTNode> body) {
        this(body.toList());
    }

    @Override
    public <T> T accept(T context, Visitor<T> visitor) {
        throw new UnsupportedOperationException("Unimplemented method 'accept'");
    }

    public List<MethodSignature> getBody() {
        return Collections.unmodifiableList(body);
    }

    @Override
    public String toString() {
        return "interfaceCons(" + Cons.stringFromList(body) + ")";
    }
}
