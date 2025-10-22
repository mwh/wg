package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.Method;
import nz.mwh.cpsgrace.objects.UserObject;

public class Block extends ASTNode {
    private List<String> parameterNames;
    private List<ASTNode> body;
    private List<String> vars = new ArrayList<>();
    private List<String> defs = new ArrayList<>();

    public Block(List<ASTNode> parameterNames, List<ASTNode> body) {
        this.parameterNames = parameterNames.stream().map(x -> {
            switch (x) {
                case IdentifierDeclaration idDec -> {
                    return idDec.getName();
                }
                default -> {
                    throw new RuntimeException("Invalid parameter in block: " + x);
                }
            }
        }).toList();
        this.body = new ArrayList<>(body);
        for (ASTNode member : body) {
            switch (member) {
                case VarDec varDec -> {
                    vars.add(varDec.getName());
                }
                case DefDec defDec -> {
                    defs.add(defDec.getName());
                }
                default -> {}
            }
        }
    }

    public List<ASTNode> getBody() {
        return body;
    }

    public List<String> getParameterNames() {
        return parameterNames;
    }

    public CPS toCPS() {
        return (ctx, blockCont) -> {
            Continuation returnContinuation = ctx.getReturnContinuation();
            UserObject obj = new UserObject();
            obj.setSurrounding(ctx.getScope());
            obj.setDebugLabel("a block");
            CPS blockBody = (reqCtx, reqCont) -> {
                Context evalCtx = reqCtx.withReturnContinuation(returnContinuation);
                PendingStep step = null;
                Continuation cont = reqCont;
                for (int j = body.size() - 1; j > 0; j--) {
                    int k = j;
                    Continuation next = cont;
                    cont = (_) -> {
                        return body.get(k).toCPS().run(evalCtx, next);
                    };
                }
                step = body.get(0).toCPS().run(evalCtx, cont);
                return step;
            };
            Method apply = new Method(this.parameterNames, blockBody, vars, defs, false);
            String name = parameterNames.isEmpty() ? "apply" : "apply(" + parameterNames.size() + ")";
            obj.addMethod(name, apply);
            return new PendingStep(ctx, blockCont, obj);
        };
    }
}
