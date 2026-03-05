package nz.mwh.cpsgrace.ast;

import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.objects.GraceType;

public class InterfaceCons extends ASTNode {
    private final List<MethSig> body;

    public InterfaceCons(List<MethSig> body) {
        this.body = body;
    }

    public List<MethSig> getBody() {
        return body;
    }

    @Override
    public CPS toCPS() {
        return (ctx, cont) -> cont.apply(new GraceType(body));
    }
}
