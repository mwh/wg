package nz.mwh.cpsgrace.ast;

import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;

public class TypeDec extends ASTNode {
    private final String name;
    private final List<ASTNode> genericParameters;
    private final ASTNode typeExpr;

    public TypeDec(String name, List<ASTNode> genericParameters, ASTNode typeExpr) {
        this.name = name;
        this.genericParameters = genericParameters;
        this.typeExpr = typeExpr;
    }

    public String getName() {
        return name;
    }

    public List<ASTNode> getGenericParameters() {
        return genericParameters;
    }

    public ASTNode getTypeExpr() {
        return typeExpr;
    }

    @Override
    public CPS toCPS() {
        CPS typeCPS = typeExpr.toCPS();
        return (ctx, cont) -> {
            return typeCPS.run(ctx, (value) -> {
                GraceObject receiver = ctx.findReceiver(name + " =(1)");
                return receiver.requestMethod(ctx, cont, name + " =(1)", List.of(value));
            });
        };
    }
}
