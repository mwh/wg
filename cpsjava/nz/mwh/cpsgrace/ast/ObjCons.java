package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.Method;
import nz.mwh.cpsgrace.objects.UserObject;

public class ObjCons extends ASTNode {
    private List<ASTNode> body;

    public ObjCons(List<ASTNode> body) {
        this.body = new ArrayList<>(body);
    }

    public List<ASTNode> getBody() {
        return body;
    }

    public CPS toCPS() {
        // go through children in order
        return (ctx, returnCont) -> {
            UserObject obj = new UserObject();
            obj.setSurrounding(ctx.getScope());
            obj.addDefaultMethods();
            // System.err.println("Constructing object with surrounding: " + ctx.getScope());
            for (ASTNode member : body) {
                switch (member) {
                    case VarDec varDec -> {
                        obj.addVar(varDec.getName());
                    }
                    case DefDec defDec -> {
                        obj.addDef(defDec.getName());
                    }
                    case TypeDec typeDec -> {
                        // Pre-populate with a GraceTypeRef placeholder so forward
                        // and circular references resolve correctly.
                        obj.addTypeDef(typeDec.getName());
                    }
                    case MethodDecl methodDecl -> {
                        obj.addMethod(methodDecl.getName(), new Method(
                            methodDecl.getParameterNames(),
                            methodDecl.getGenericParameterNames(),
                            methodDecl.bodyCPS(),
                            methodDecl.getVarNames(),
                            methodDecl.getDefNames(),
                            true
                        ).described(methodDecl.getName()));
                    }
                    case ImportStmt importStmt -> {
                        obj.addDef(importStmt.getAsName().getName());
                    }
                    default -> {}
                }
            }

            // Lift type declarations to run before all other body statements,
            // matching the java/ implementation's two-pass evaluation order.
            List<ASTNode> execOrder = new ArrayList<>();
            for (ASTNode member : body) {
                if (member instanceof TypeDec) execOrder.add(member);
            }
            for (ASTNode member : body) {
                if (!(member instanceof TypeDec)) execOrder.add(member);
            }

            if (execOrder.isEmpty()) {
                return returnCont.apply(obj);
            }

            PendingStep step = null;
            Continuation cont = _ -> returnCont.apply(obj);
            Context bodyContext = ctx.withSelfScope(obj);
            for (int i = execOrder.size() - 1; i > 0; i--) {
                int j = i;
                Continuation next = cont;
                cont = (_) -> {
                    return execOrder.get(j).toCPS().run(bodyContext, next);
                };
            }
            step = execOrder.get(0).toCPS().run(bodyContext, cont);
            return step;
        };
    }
}
