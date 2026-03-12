package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.GraceLineup;

public class Lineup extends ASTNode {
    private List<ASTNode> elements;

    public Lineup(List<ASTNode> elements) {
        this.elements = elements;
    }

    public List<ASTNode> getElements() {
        return elements;
    }

    public CPS toCPS() {
        List<CPS> elementCPS = elements.stream().map(ASTNode::toCPS).toList();
        return (ctx, cont) -> {
            List<GraceObject> values = new ArrayList<>();
            return evalElements(ctx, cont, elementCPS, values, 0);
        };
    }

    private static PendingStep evalElements(Context ctx, Continuation cont, List<CPS> elementCPS, List<GraceObject> values, int index) {
        if (index >= elementCPS.size()) {
            return cont.returning(ctx, new GraceLineup(values));
        }
        return elementCPS.get(index).run(ctx, (GraceObject val) -> {
            values.add(val);
            return evalElements(ctx, cont, elementCPS, values, index + 1);
        });
    }
}
