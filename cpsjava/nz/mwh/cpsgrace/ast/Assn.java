package nz.mwh.cpsgrace.ast;

import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;

public class Assn extends ASTNode {
    private final ASTNode target;
    private final ASTNode value;

    public Assn(ASTNode target, ASTNode value) {
        this.target = target;
        this.value = value;
    }

    public ASTNode getTarget() {
        return target;
    }

    public ASTNode getValue() {
        return value;
    }

    public CPS toCPS() {
        CPS targetCPS;
        String name;
        switch (target) {
            case LexReq lr -> {
                targetCPS = null;
                name = lr.getName() + ":=(1)";
            }
            case DotReq dr -> {
                targetCPS = dr.getReceiver().toCPS();
                name = dr.getName() + ":=(1)";
            }
            default -> throw new UnsupportedOperationException("Unsupported assignment target: " + target.getClass().descriptorString());
        }
        CPS valueCPS = value.toCPS();
        return (ctx, cont) -> {
            GraceObject receiver;
            if (targetCPS == null) {
                // Lexical request
                receiver = ctx.findReceiver(name);
                if (receiver == null) {
                    throw new RuntimeException("No receiver found for assignment to " + name);
                }
                return valueCPS.run(ctx, (value) -> {
                    return receiver.requestMethod(ctx, cont, name, List.of(value));
                });
            }

            // Dot request
            return targetCPS.run(ctx, (recv) -> {
                return valueCPS.run(ctx, (value) -> {
                    return recv.requestMethod(ctx, cont, name, List.of(value));
                });
            });
            
        };
    }
}
