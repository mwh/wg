package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;
import java.util.Collections;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.GraceMatchResult;
import nz.mwh.cpsgrace.objects.GracePatternOr;
import nz.mwh.cpsgrace.objects.Method;
import nz.mwh.cpsgrace.objects.UserObject;

public class Block extends ASTNode {
    private List<String> parameterNames;
    private List<ASTNode> parameterTypes; // null entry = no type annotation
    private List<ASTNode> body;
    private List<String> vars = new ArrayList<>();
    private List<String> defs = new ArrayList<>();

    public Block(List<ASTNode> parameterNodes, List<ASTNode> body) {
        this.parameterNames = new ArrayList<>();
        this.parameterTypes = new ArrayList<>();
        for (ASTNode x : parameterNodes) {
            switch (x) {
                case IdentifierDeclaration idDec -> {
                    parameterNames.add(idDec.getName());
                    parameterTypes.add(idDec.getType()); // may be null
                }
                default -> {
                    throw new RuntimeException("Invalid parameter in block: " + x);
                }
            }
        }
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
                Context evalCtx = reqCtx.withReturnContinuation(returnContinuation).withSelf(ctx.getSelf());
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
            Method apply = new Method(this.parameterNames, Collections.emptyList(), blockBody, vars, defs, false);
            String applyName = parameterNames.isEmpty() ? "apply" : "apply(" + parameterNames.size() + ")";
            obj.addMethod(applyName, apply);

            // Add match(1) for single-parameter blocks
            if (parameterNames.size() == 1) {
                ASTNode typeAnnotation = parameterTypes.get(0);
                UserObject blockScope = ctx.getScope();
                obj.addMethod("match(1)", Method.java((matchCtx, matchCont, self, matchArgs) -> {
                    GraceObject target = matchArgs.get(0);
                    if (typeAnnotation != null) {
                        // Evaluate the type annotation in the block's lexical scope
                        Context annotCtx = matchCtx.withScope(blockScope);
                        return typeAnnotation.toCPS().run(annotCtx, (GraceObject patternObj) -> {
                            // Call match(target) on the pattern
                            return patternObj.requestMethod(matchCtx, (GraceObject matchResultObj) -> {
                                GraceMatchResult mr = GraceMatchResult.assertMatchResult(matchResultObj);
                                if (mr.isSuccess()) {
                                    // Apply the block body with the matched value
                                    return obj.requestMethod(matchCtx, (GraceObject bodyResult) -> {
                                        return matchCont.returning(matchCtx, new GraceMatchResult(true, bodyResult));
                                    }, applyName, List.of(target));
                                }
                                return matchCont.returning(matchCtx, new GraceMatchResult(false, target));
                            }, "match(1)", List.of(target));
                        });
                    } else {
                        // No type annotation: unconditionally apply block with target
                        return obj.requestMethod(matchCtx, (GraceObject bodyResult) -> {
                            return matchCont.returning(matchCtx, new GraceMatchResult(true, bodyResult));
                        }, applyName, List.of(target));
                    }
                }));

                obj.addMethod("|(1)", Method.java((orCtx, orCont, self, orArgs) -> {
                    return orCont.returning(orCtx, new GracePatternOr(obj, orArgs.get(0)));
                }));
            }



            return new PendingStep(ctx, blockCont, obj);
        };
    }
}
