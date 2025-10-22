package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;
import nz.mwh.cpsgrace.objects.GraceBoolean;

public class LexReq extends ASTNode {
    private String name;
    private List<ASTNode> arguments;

    public LexReq(List<Part> parts) {
        StringBuilder sb = new StringBuilder();
        arguments = new ArrayList<ASTNode>();
        for (Part p : parts) {
            List<ASTNode> args = p.getArguments();
            sb.append(p.getName());
            if (!args.isEmpty()) {
                sb.append('(');
                sb.append(args.size());
                sb.append(')');
            }
            arguments.addAll(args);
        }
        name = sb.toString();
    }

    public List<ASTNode> getArguments() {
        return arguments;
    }

    public String getName() {
        return name;
    }

    public CPS toCPS() {
        List<CPS> argCPS = arguments.stream().map(ASTNode::toCPS).toList();
        return new CPS() {
            public PendingStep run(Context ctx, Continuation returnCont) {
                return new PendingStep(ctx, (_) -> {
                    List<GraceObject> requestArgs = new ArrayList<>();
                    Continuation finalCont = (_) -> {
                        switch (name) {
                            case "self" -> {
                                return returnCont.apply(ctx.getSelf());
                            }
                            case "true" -> {
                                return returnCont.apply(GraceBoolean.TRUE);
                            }
                            case "false" -> {
                                return returnCont.apply(GraceBoolean.FALSE);
                            }
                        }
                        
                        GraceObject receiver = ctx.findReceiver(name);
                        if (receiver != null) {
                            return receiver.requestMethod(ctx, returnCont, name, requestArgs);
                        }

                        System.out.println("no such method " + name);
                        return null;
                    };
                    Continuation cont = finalCont;
                    for (int i = arguments.size() - 1; i >= 0; i--) {
                        int j = i;
                        Continuation next = cont;
                        cont = (_) -> {
                            return argCPS.get(j).run(ctx, (val) -> {
                                requestArgs.add(j, val);
                                return new PendingStep(ctx, next, null);
                            });
                        };
                    }
                    return new PendingStep(ctx, cont, null);

                }, null);
            }
        };
    }

}
