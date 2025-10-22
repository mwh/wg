package nz.mwh.cpsgrace;

import java.util.ArrayDeque;

import nz.mwh.cpsgrace.objects.GraceBoolean;
import nz.mwh.cpsgrace.objects.Method;
import nz.mwh.cpsgrace.objects.UserObject;

public class Start {
    
    public static UserObject prelude;

    public static void main(String[] args) {
        new Start().run();
    }
    
    public void run() {
        Context startContext = new Context();
        ArrayDeque<PendingStep> queue = new ArrayDeque<>();

        prelude = startContext.extendScope("prelude"); 
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
                return thenBlock.requestMethod(ctx, _ -> cont.apply(GraceObject.DONE), "apply", java.util.List.of());
            } else {
                return cont.apply(GraceObject.DONE);
            }
        }));
        prelude.addMethod("if(1)then(1)else(1)", Method.java((ctx, cont, _, args) -> {
            GraceBoolean condition = GraceBoolean.assertBoolean(args.get(0));
            GraceObject thenBlock = args.get(1);
            GraceObject elseBlock = args.get(2);
            if (condition.getValue()) {
                return thenBlock.requestMethod(ctx, cont, "apply", java.util.List.of());
            } else {
                return elseBlock.requestMethod(ctx, cont, "apply", java.util.List.of());
            }
        }));
        Method ifThenCombinedMethod = Method.java((ctx, cont, _, args) -> {
            GraceBoolean condition = GraceBoolean.assertBoolean(args.get(0));
            GraceObject thenBlock = args.get(1);
            if (condition.getValue()) {
                return thenBlock.requestMethod(ctx, cont, "apply", java.util.List.of());
            }
            Continuation next;
            int lastElseifIndex;
            if (args.size() % 2 == 1) {
                // We have an else block
                GraceObject elseBlock = args.get(args.size() - 1);
                next = (_) -> elseBlock.requestMethod(ctx, cont, "apply", java.util.List.of());
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
                            return elifBlock.requestMethod(ctx, cont, "apply", java.util.List.of());
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
            return iterable.requestMethod(ctx, _ -> cont.apply(GraceObject.DONE), "each", java.util.List.of(loopBlock));
        }));

        prelude.addMethod("while(1)do(1)", Method.java((ctx, cont, _, args) -> {
            return whileDo(ctx, cont, args.get(0), args.get(1));
        }));
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

    PendingStep whileDo(Context ctx, Continuation cont, GraceObject conditionBlock, GraceObject loopBlock) {
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
