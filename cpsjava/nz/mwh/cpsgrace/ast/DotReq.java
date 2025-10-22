package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class DotReq extends ASTNode {
    private ASTNode receiver;
    private String name;
    private List<ASTNode> arguments;

    public DotReq(ASTNode receiver, List<Part> parts) {
        this.receiver = receiver;
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

    public ASTNode getReceiver() {
        return receiver;
    }

    public CPS toCPS() {
        CPS receiverCPS = receiver.toCPS();
        List<CPS> argCPS = arguments.stream().map(ASTNode::toCPS).toList();
        return new CPS() {
            public PendingStep run(Context ctx, Continuation returnCont) {
                return new PendingStep(ctx, (_) -> {
                    // first evaluate receiver
                    PendingStep receiverStep = receiverCPS.run(ctx, (GraceObject recv) -> {
                        List<GraceObject> requestArgs = new ArrayList<>();
                        Continuation invokeCont = (_) -> {
                            return recv.requestMethod(ctx, returnCont, name, requestArgs);
                        };
                        Continuation cont = invokeCont;
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
                    });
                    return receiverStep;
                }, null);
            }
        };
    }
}
