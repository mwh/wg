package nz.mwh.cpsgrace;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayDeque;
import java.util.List;

import nz.mwh.cpsgrace.ast.ASTNode;
import nz.mwh.cpsgrace.ast.Converter;
import nz.mwh.cpsgrace.objects.GraceBoolean;
import nz.mwh.cpsgrace.objects.GraceMatchResult;
import nz.mwh.cpsgrace.objects.GracePatternOr;
import nz.mwh.cpsgrace.objects.GraceString;
import nz.mwh.cpsgrace.objects.Method;
import nz.mwh.cpsgrace.objects.UserObject;

public class Start {
    
    public static UserObject prelude;

    public static void main(String[] args) {
        if (args.length > 0) {
            String fileName = null;
            String mode = "run";
            for (int i = 0; i < args.length; i++) {
                switch (args[i]) {
                    case "-p":
                        mode = "print-ast";
                        break;
                    case "-P":
                        mode = "concise-ast";
                        break;
                    default:
                        fileName = args[i];
                        break;
                }
            }
            if (fileName != null) {
                GraceObject graceAST = parseToGraceAST(fileName);
                if (graceAST == null) {
                    System.err.println("Failed to parse file " + fileName);
                    return;
                }
                if (mode.equals("print-ast")) {
                    Context ctx = new Context();
                    addPrelude(ctx);
                    PendingStep step = graceAST.requestMethod(ctx, (GraceObject gs) -> {
                        String astStr = GraceString.assertString(gs).toString();
                        System.out.println(astStr);
                        return null;
                    }, "asString", List.of(), java.util.List.of());
                    while (step != null) {
                        step = step.go();
                    }
                } else if (mode.equals("concise-ast")) {
                    Context ctx = new Context();
                    addPrelude(ctx);
                    PendingStep step = graceAST.requestMethod(ctx, (GraceObject gs) -> {
                        String astStr = GraceString.assertString(gs).toString();
                        System.out.println(astStr);
                        return null;
                    }, "concise", List.of(), java.util.List.of());
                    while (step != null) {
                        step = step.go();
                    }
                } else {
                    ASTNode prog = graceASTtoASTNode(graceAST);
                    Context ctx = new Context();
                    addPrelude(ctx);
                    PendingStep step = prog.toCPS().run(ctx, (GraceObject _) -> {
                        return null;
                    });
                    while (step != null) {
                        step = step.go();
                    }
                }
            } else {
                System.out.println("No input file specified.");
            }
        } else {
            new Start().run();
        }
    }

    public static GraceObject parseToGraceAST(String filename) {
        try {
            GraceObject[] returnVal = new GraceObject[1];
            Context ctx = new Context();
            addPrelude(ctx);
            String content = Files.readString(Path.of(filename));
            PendingStep step = TheProgram.getModuleAST("parser").toCPS().run(ctx, (GraceObject o) -> {
                return o.requestMethod(ctx, (result) -> {
                    returnVal[0] = result;
                    return null;
                }, "parseModule(2)", List.of(new GraceString(filename), new GraceString(content)));
            });
            while (step != null) {
                step = step.go();
            }
            return returnVal[0];
        } catch (IOException e) {
            System.err.println("Error reading file " + filename);
            return null;
        }
    }

    public static ASTNode graceASTtoASTNode(GraceObject graceAST) {
        Converter c = new Converter();
        return c.convertNode(graceAST);
    }

    public static ASTNode parseFile(String filename) {
        GraceObject graceAST = parseToGraceAST(filename);
        return graceASTtoASTNode(graceAST);
    }

    public static GraceObject addPrelude(Context context) {
        prelude = context.extendScope("prelude"); 
        prelude.addMethod("print(1)", Method.java((ctx, cont, _, args) -> {
            return args.get(0).requestMethod(ctx, (GraceObject obj) -> {
                System.out.println(obj);
                return cont.returning(ctx, GraceObject.DONE);
            }, "asString", java.util.List.of());
        }));
        prelude.addMethod("getFileContents(1)", Method.java((ctx, cont, _, args) -> {
            GraceObject filenameObj = args.get(0);
            return filenameObj.requestMethod(ctx, (GraceObject filenameStrObj) -> {
                String filename = ((nz.mwh.cpsgrace.objects.GraceString)filenameStrObj).toString();
                try {
                    String content = java.nio.file.Files.readString(java.nio.file.Path.of(filename));
                    return cont.returning(ctx, new nz.mwh.cpsgrace.objects.GraceString(content));
                } catch (java.io.IOException e) {
                    throw new RuntimeException("Error reading file " + filename, e);
                }
            }, "asString", java.util.List.of());
        }));
        prelude.addMethod("if(1)then(1)", Method.java((ctx, cont, _, args) -> {
            GraceBoolean condition = GraceBoolean.assertBoolean(args.get(0));
            GraceObject thenBlock = args.get(1);
            if (condition.getValue()) {
                return thenBlock.requestMethod(ctx, _ -> cont.apply(GraceObject.DONE), "apply", java.util.List.of(), java.util.List.of());
            } else {
                return cont.apply(GraceObject.DONE);
            }
        }));
        prelude.addMethod("if(1)then(1)else(1)", Method.java((ctx, cont, _, args) -> {
            GraceBoolean condition = GraceBoolean.assertBoolean(args.get(0));
            GraceObject thenBlock = args.get(1);
            GraceObject elseBlock = args.get(2);
            if (condition.getValue()) {
                return thenBlock.requestMethod(ctx, cont, "apply", java.util.List.of(), java.util.List.of());
            } else {
                return elseBlock.requestMethod(ctx, cont, "apply", java.util.List.of(), java.util.List.of());
            }
        }));
        Method ifThenCombinedMethod = Method.java((ctx, cont, _, args) -> {
            GraceBoolean condition = GraceBoolean.assertBoolean(args.get(0));
            GraceObject thenBlock = args.get(1);
            if (condition.getValue()) {
                return thenBlock.requestMethod(ctx, cont, "apply", java.util.List.of(), java.util.List.of());
            }
            Continuation next;
            int lastElseifIndex;
            if (args.size() % 2 == 1) {
                // We have an else block
                GraceObject elseBlock = args.get(args.size() - 1);
                next = (_) -> elseBlock.requestMethod(ctx, cont, "apply", java.util.List.of(), java.util.List.of());
                lastElseifIndex = args.size() - 3;
            } else {
                next = (_) -> cont.apply(GraceObject.DONE);
                lastElseifIndex = args.size() - 2;
            }
            // Loop backwards through elifs
            for (int i = lastElseifIndex; i >= 2; i -= 2) {
                GraceObject elifConditionBlock = args.get(i);
                GraceObject elifBlock = args.get(i + 1);
                Continuation currentNext = next;
                next = (_) -> {
                    return elifConditionBlock.requestMethod(ctx, (GraceObject elifConditionResult) -> {
                        GraceBoolean elifCondition = GraceBoolean.assertBoolean(elifConditionResult);
                        if (elifCondition.getValue()) {
                            return elifBlock.requestMethod(ctx, cont, "apply", java.util.List.of(), java.util.List.of());
                        } else {
                            return currentNext.apply(GraceObject.DONE);
                        }
                    }, "apply", java.util.List.of());
                };
            }
            return next.apply(GraceObject.DONE);
        });
        prelude.addMethod("if(1)then(1)elseif(1)then(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)else(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)", ifThenCombinedMethod);
        prelude.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", ifThenCombinedMethod);
        
        prelude.addMethod("for(1)do(1)", Method.java((ctx, cont, _, args) -> {
            GraceObject iterable = args.get(0);
            GraceObject loopBlock = args.get(1);
            return iterable.requestMethod(ctx, _ -> cont.apply(GraceObject.DONE), "each(1)", java.util.List.of(loopBlock), java.util.List.of());
        }));

        prelude.addMethod("while(1)do(1)", Method.java((ctx, cont, _, args) -> {
            return whileDo(ctx, cont, args.get(0), args.get(1));
        }));

        prelude.addMethod("getStackTrace", Method.java((ctx, cont, _, _) -> {
            CallStackItem item = ctx.getCallStack();
            StringBuilder sb = new StringBuilder();
            item = item.caller(); // skip request to this method
            while (item != null) {
                sb.append("  from request to ").append(item.functionName()).append("\n");
                item = item.caller();
            }
            return cont.returning(ctx, new GraceString(sb.toString()));
        }));

        prelude.addMethod("successfulMatch(1)", Method.java((ctx, cont, _, args) -> {
            GraceMatchResult mr = new GraceMatchResult(true, args.get(0));
            return cont.returning(ctx, mr);
        }));

        prelude.addMethod("failedMatch(1)", Method.java((ctx, cont, _, args) -> {
            GraceMatchResult mr = new GraceMatchResult(false, args.get(0));
            return cont.returning(ctx, mr);
        }));

        // match(1)case(1) through match(1)...case(1)*30
        // Each case argument is a block (or pattern). We try each case's match(target) in order.
        // The first successful match result is returned.
        String matchCaseName = "match(1)";
        for (int i = 0; i < 30; i++) {
            matchCaseName += "case(1)";
            final String finalName = matchCaseName;
            prelude.addMethod(finalName, Method.java((ctx, cont, _, args) -> {
                GraceObject target = args.get(0);
                // args.get(1) is the first case, args.get(2) the second, etc.
                return tryMatchCases(ctx, cont, target, args, 1);
            }));
        }

        // reset(1) - delimited continuation delimiter
        prelude.addMethod("reset(1)", Method.java((ctx, cont, _, args) -> {
            GraceObject bodyBlock = args.get(0);
            // Mutable cell: promptTarget[0] starts as the outer continuation.
            // The prompt continuation reads from this cell, so k.apply can redirect it.
            Continuation[] promptTarget = { cont };
            Continuation promptCont = (GraceObject bodyResult) -> {
                return promptTarget[0].apply(bodyResult);
            };
            // Install the prompt target in the context so shift/k.apply can access it
            Context resetCtx = ctx.withResetPromptTarget(promptTarget);
            return bodyBlock.requestMethod(resetCtx, promptCont, "apply", java.util.List.of(), java.util.List.of());
        }));

        // shift(1) - capture the delimited continuation up to the nearest reset
        prelude.addMethod("shift(1)", Method.java((ctx, cont, _, args) -> {
            GraceObject shiftBody = args.get(0);
            Continuation[] promptTarget = ctx.getResetPromptTarget();
            if (promptTarget == null) {
                throw new RuntimeException("shift called outside of any reset");
            }
            // Read the current abort target (outer continuation of enclosing reset)
            Continuation shiftAbortTarget = promptTarget[0];
            // The captured continuation: from the shift point through to the prompt
            Continuation capturedK = cont;
            // Wrap captured continuation as a Grace object with apply(1)
            UserObject reifiedCont = new UserObject();
            reifiedCont.setDebugLabel("a reified continuation");
            reifiedCont.addMethod("apply(1)", Method.java((applyCtx, applyCont, _, applyArgs) -> {
                GraceObject resumeValue = applyArgs.get(0);
                // Redirect the prompt to the apply caller's continuation.
                // When the resumed computation reaches the prompt, it will
                // forward to applyCont, making the result the return value of k.apply(v).
                promptTarget[0] = applyCont;
                return new PendingStep(applyCtx, capturedK, resumeValue);
            }));
            // Evaluate the shift body; its result goes directly to the abort target
            // (the outer continuation of the enclosing reset), bypassing the prompt.
            return shiftBody.requestMethod(ctx, shiftAbortTarget, "apply(1)", java.util.List.of(reifiedCont), java.util.List.of());
        }));

        // Exception object with refine(1) and raise(1)
        UserObject exceptionObj = new UserObject();
        exceptionObj.setDebugLabel("Exception");
        exceptionObj.addMethod("refine(1)", Method.java((ctx, cont, self, args) -> {
            GraceObject nameObj = args.get(0);
            return nameObj.requestMethod(ctx, (GraceObject nameStr) -> {
                String name = nameStr.toString();
                UserObject refinedExn = new UserObject();
                refinedExn.setDebugLabel(name);
                refinedExn.addMethod("raise(1)", Method.java((raiseCtx, raiseCont, _, raiseArgs) -> {
                    GraceObject messageObj = raiseArgs.get(0);
                    return messageObj.requestMethod(raiseCtx, (GraceObject msgStr) -> {
                        UserObject exnValue = new UserObject();
                        exnValue.setDebugLabel("exception: " + name);
                        exnValue.addMethod("message", Method.java((c, k, _, _) -> k.returning(c, msgStr)));
                        exnValue.addMethod("name", Method.java((c, k, _, _) -> k.returning(c, new GraceString(name))));
                        exnValue.addMethod("asString", Method.java((c, k, _, _) -> k.returning(c, new GraceString(name + ": " + msgStr))));
                        Continuation exnK = raiseCtx.getExceptionContinuation();
                        if (exnK != null) {
                            return exnK.apply(exnValue);
                        }
                        throw new RuntimeException("Unhandled exception: " + name + ": " + msgStr);
                    }, "asString", java.util.List.of());
                }));
                refinedExn.addMethod("match(1)", Method.java((matchCtx, matchCont, _, matchArgs) -> {
                    // For now, exception types always match
                    GraceObject target = matchArgs.get(0);
                    return matchCont.returning(matchCtx, new GraceMatchResult(true, target));
                }));
                return cont.returning(ctx, refinedExn);
            }, "asString", java.util.List.of());
        }));
        exceptionObj.addMethod("raise(1)", Method.java((ctx, cont, _, args) -> {
            GraceObject messageObj = args.get(0);
            return messageObj.requestMethod(ctx, (GraceObject msgStr) -> {
                UserObject exnValue = new UserObject();
                exnValue.setDebugLabel("exception");
                exnValue.addMethod("message", Method.java((c, k, _, _) -> k.returning(c, msgStr)));
                exnValue.addMethod("name", Method.java((c, k, _, _) -> k.returning(c, new GraceString("Exception"))));
                exnValue.addMethod("asString", Method.java((c, k, _, _) -> k.returning(c, new GraceString("Exception: " + msgStr))));
                Continuation exnK = ctx.getExceptionContinuation();
                if (exnK != null) {
                    return exnK.apply(exnValue);
                }
                throw new RuntimeException("Unhandled exception: " + msgStr);
            }, "asString", java.util.List.of());
        }));
        prelude.addMethod("Exception", Method.java((ctx, cont, _, _) -> cont.returning(ctx, exceptionObj)));

        // try(1)catch(1)
        prelude.addMethod("try(1)catch(1)", Method.java((ctx, cont, _, args) -> {
            GraceObject tryBlock = args.get(0);
            GraceObject catchBlock = args.get(1);
            Continuation exnCont = (GraceObject exnValue) -> {
                return catchBlock.requestMethod(ctx, cont, "apply(1)", java.util.List.of(exnValue), java.util.List.of());
            };
            Context tryCtx = ctx.withExceptionContinuation(exnCont);
            return tryBlock.requestMethod(tryCtx, cont, "apply", java.util.List.of(), java.util.List.of());
        }));

        return prelude;
    }
    
    private static PendingStep tryMatchCases(Context ctx, Continuation cont, GraceObject target, java.util.List<GraceObject> args, int caseIndex) {
        if (caseIndex >= args.size()) {
            // No case matched - return a failed match result
            return cont.returning(ctx, new GraceMatchResult(false, target));
        }
        GraceObject pattern = args.get(caseIndex);
        return pattern.requestMethod(ctx, (GraceObject matchResultObj) -> {
            GraceMatchResult mr = GraceMatchResult.assertMatchResult(matchResultObj);
            if (mr.isSuccess()) {
                return cont.returning(ctx, mr);
            }
            return tryMatchCases(ctx, cont, target, args, caseIndex + 1);
        }, "match(1)", java.util.List.of(target));
    }

    public void run() {
        Context startContext = new Context();
        ArrayDeque<PendingStep> queue = new ArrayDeque<>();

        addPrelude(startContext);
        
        // var sl = new nz.mwh.cpsgrace.ast.StrLit("hello");
        // CPS c1 = sl.toCPS();

        // var lr = new nz.mwh.cpsgrace.ast.LexReq(List.of(new Part("print", List.of(sl))));
        // CPS c2 = lr.toCPS();

        // queue.push(c2.run(startContext, (GraceObject o) -> { System.out.println(o); return null; }));
        queue.push(TheProgram.program.toCPS().run(startContext, (GraceObject o) -> { System.out.println("final output: " + o); return null; }));
        while (!queue.isEmpty()) {
            var step = queue.poll();
            var ret = step.go();
            if (ret != null)
                queue.push(ret);
        }
    }

    private static PendingStep whileDo(Context ctx, Continuation cont, GraceObject conditionBlock, GraceObject loopBlock) {
        return conditionBlock.requestMethod(ctx, (GraceObject conditionResult) -> {
            GraceBoolean condition = GraceBoolean.assertBoolean(conditionResult);
            if (condition.getValue()) {
                return loopBlock.requestMethod(ctx, (_) -> {
                    return whileDo(ctx, cont, conditionBlock, loopBlock);
                }, "apply", java.util.List.of());
            } else {
                return cont.apply(GraceObject.DONE);
            }
        }, "apply", java.util.List.of());
    }
}
