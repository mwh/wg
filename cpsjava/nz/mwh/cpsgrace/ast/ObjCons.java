package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
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

    /**
     * Register a method declaration on obj, and if the method's last body
     * statement is an ObjCons, also register an inherit variant.
     */
    private static void registerMethod(UserObject obj, MethodDecl methodDecl) {
        obj.addMethod(methodDecl.getName(), new Method(
            methodDecl.getParameterNames(),
            methodDecl.getGenericParameterNames(),
            methodDecl.bodyCPS(),
            methodDecl.getVarNames(),
            methodDecl.getDefNames(),
            true
        ).described(methodDecl.getName()));

        // Generate inherit variant if last body is ObjCons
        List<? extends ASTNode> mdBody = methodDecl.getBody();
        if (!mdBody.isEmpty() && mdBody.getLast() instanceof ObjCons lastObjCons) {
            String inheritName = methodDecl.getName() + "inherit(1)";
            obj.addMethod(inheritName, Method.java((ctx2, returnCont2, self2, args2) -> {
                // Last argument is the inheriting object
                UserObject inheritObject = (UserObject) args2.get(args2.size() - 1);
                List<GraceObject> normalArgs = args2.subList(0, args2.size() - 1);

                // Set up body context as Method.invoke would
                Context bodyContext = ctx2.withSelf((UserObject) self2);
                bodyContext = bodyContext.withReturnContinuation(returnCont2);
                UserObject localScope = bodyContext.extendScope("inherit " + methodDecl.getName());

                List<String> paramNames = methodDecl.getParameterNames();
                for (int i = 0; i < paramNames.size(); i++) {
                    bodyContext.bindLocalName(paramNames.get(i), normalArgs.get(i));
                }
                List<String> genParamNames = methodDecl.getGenericParameterNames();
                for (int i = 0; i < genParamNames.size(); i++) {
                    bodyContext.bindLocalName(genParamNames.get(i), new UserObject());
                }
                for (String varName : methodDecl.getVarNames()) {
                    localScope.addVar(varName);
                }
                for (String defName : methodDecl.getDefNames()) {
                    localScope.addDef(defName);
                }

                Context evalCtx = bodyContext;

                // After running body[0..n-2], run lastObjCons in inherit mode
                Continuation finalCont = (_) -> {
                    return lastObjCons.toInheritCPS().run(evalCtx.withSelfScope(inheritObject), returnCont2);
                };

                if (mdBody.size() == 1) {
                    // Only the ObjCons
                    return finalCont.apply(GraceObject.DONE);
                }

                Continuation cont = finalCont;
                for (int j = mdBody.size() - 2; j > 0; j--) {
                    int k = j;
                    Continuation next = cont;
                    cont = (_) -> mdBody.get(k).toCPS().run(evalCtx, next);
                }
                return mdBody.get(0).toCPS().run(evalCtx, cont);
            }).described("inherit " + methodDecl.getName()));
        }
    }

    public CPS toCPS() {
        return buildCPS(false);
    }

    /**
     * CPS for inherit mode: uses context's self as the target object,
     * only adds methods/defs/vars not already present.
     */
    public CPS toInheritCPS() {
        return buildCPS(true);
    }

    private CPS buildCPS(boolean inheritMode) {
        return (ctx, returnCont) -> {
            UserObject obj;
            if (inheritMode) {
                obj = ctx.getSelf();
            } else {
                obj = new UserObject();
                obj.setSurrounding(ctx.getScope());
                obj.addDefaultMethods();
            }

            // Collect use/inherit statements and other members
            List<UseStmt> useStmts = new ArrayList<>();
            InheritStmt inheritStmt = null;

            for (ASTNode member : body) {
                switch (member) {
                    case UseStmt us -> useStmts.add(us);
                    case InheritStmt is -> inheritStmt = is;
                    default -> {}
                }
            }

            // Phase A: Process use statements (CPS - evaluate mixin exprs in enclosing scope)
            // Phase B: Register own methods/defs/vars (synchronous - override use methods)
            // Phase C: Handle inherit statement (CPS)
            // Phase D: Execute body statements (CPS)

            final InheritStmt finalInheritStmt = inheritStmt;

            // Build continuation chain from end to start:
            // Phase D: execute body statements
            Continuation phaseD = (_) -> {
                // Lift type declarations first, then rest (excluding types)
                List<ASTNode> execOrder = new ArrayList<>();
                for (ASTNode member : body) {
                    if (member instanceof TypeDec) execOrder.add(member);
                }
                for (ASTNode member : body) {
                    if (!(member instanceof TypeDec) && !(member instanceof MethodDecl)
                            && !(member instanceof UseStmt) && !(member instanceof InheritStmt)) {
                        execOrder.add(member);
                    }
                }

                if (execOrder.isEmpty()) {
                    return returnCont.apply(obj);
                }

                Context bodyContext = inheritMode ? ctx.withSelfScope(obj) : ctx.withSelfScope(obj);
                Continuation cont = _ -> returnCont.apply(obj);
                for (int i = execOrder.size() - 1; i > 0; i--) {
                    int j = i;
                    Continuation next = cont;
                    cont = (_2) -> execOrder.get(j).toCPS().run(bodyContext, next);
                }
                return execOrder.get(0).toCPS().run(bodyContext, cont);
            };

            // Phase C: handle inherit
            Continuation phaseC;
            if (finalInheritStmt != null) {
                phaseC = (_) -> {
                    ASTNode expr = finalInheritStmt.getExpression();
                    if (expr instanceof LexReq lr) {
                        // Build the inherit variant call
                        String inheritMethodName = lr.getName() + "inherit(1)";
                        List<CPS> argCPS = lr.getArguments().stream().map(ASTNode::toCPS).toList();

                        // Evaluate arguments, then call inherit variant
                        List<GraceObject> requestArgs = new ArrayList<>();
                        Continuation invokeCont = (_2) -> {
                            requestArgs.add(obj); // append inheriting object
                            GraceObject receiver = ctx.findReceiver(inheritMethodName);
                            if (receiver == null) {
                                throw new RuntimeException("Cannot inherit: no inheritable method '" + lr.getName() + "' found (not fresh)");
                            }
                            return receiver.requestMethod(ctx.withCall(inheritMethodName, lr.getPosition()), (inheritResult) -> {
                                return phaseD.apply(GraceObject.DONE);
                            }, inheritMethodName, requestArgs);
                        };
                        Continuation cont = invokeCont;
                        for (int i = argCPS.size() - 1; i >= 0; i--) {
                            int j = i;
                            Continuation next = cont;
                            cont = (_2) -> {
                                return argCPS.get(j).run(ctx, (val) -> {
                                    requestArgs.add(j, val);
                                    return new PendingStep(ctx, next, null);
                                });
                            };
                        }
                        return new PendingStep(ctx, cont, null);
                    } else {
                        throw new RuntimeException("Invalid inherit statement: parent must be a request");
                    }
                };
            } else {
                phaseC = phaseD;
            }

            // Phase B: register own methods/defs/vars
            Continuation phaseB = (_) -> {
                for (ASTNode member : body) {
                    switch (member) {
                        case VarDec varDec -> {
                            if (!inheritMode || !obj.hasMethod(varDec.getName())) {
                                obj.addVar(varDec.getName());
                            }
                        }
                        case DefDec defDec -> {
                            if (!inheritMode || !obj.hasMethod(defDec.getName())) {
                                obj.addDef(defDec.getName());
                            }
                        }
                        case TypeDec typeDec -> {
                            if (!inheritMode || !obj.hasMethod(typeDec.getName())) {
                                obj.addTypeDef(typeDec.getName());
                            }
                        }
                        case MethodDecl methodDecl -> {
                            if (!inheritMode || !obj.hasMethod(methodDecl.getName())) {
                                registerMethod(obj, methodDecl);
                            }
                        }
                        case ImportStmt importStmt -> {
                            if (!inheritMode || !obj.hasMethod(importStmt.getAsName().getName())) {
                                obj.addDef(importStmt.getAsName().getName());
                            }
                        }
                        default -> {}
                    }
                }
                return phaseC.apply(GraceObject.DONE);
            };

            // Phase A: process use statements
            if (useStmts.isEmpty()) {
                return phaseB.apply(GraceObject.DONE);
            }

            // Chain use statement evaluations
            Continuation afterUses = phaseB;
            for (int i = useStmts.size() - 1; i >= 0; i--) {
                UseStmt us = useStmts.get(i);
                Continuation next = afterUses;
                afterUses = (_) -> {
                    CPS exprCPS = us.getExpression().toCPS();
                    return exprCPS.run(ctx, (GraceObject mixinObj) -> {
                        if (mixinObj instanceof UserObject mixin) {
                            if (!mixin.isStateless()) {
                                throw new RuntimeException("Can only use stateless objects as mixins");
                            }
                            obj.useObject(mixin);
                        } else {
                            throw new RuntimeException("Can only use objects as mixins");
                        }
                        return next.apply(GraceObject.DONE);
                    });
                };
            }
            return afterUses.apply(GraceObject.DONE);
        };
    }
}
