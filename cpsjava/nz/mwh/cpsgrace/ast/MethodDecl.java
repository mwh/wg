package nz.mwh.cpsgrace.ast;

import java.util.ArrayList;
import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class MethodDecl extends ASTNode {
    private String name;
    private List<ASTNode> parameters;
    private ASTNode returnType;
    private List<ASTNode> annotations;
    private List<ASTNode> body;

    private List<String> parameterNames = new ArrayList<>();
    private List<String> varNames = new ArrayList<>();
    private List<String> defNames = new ArrayList<>();

    public MethodDecl(List<Part> parts, ASTNode returnType, List<ASTNode> annotations, List<ASTNode> body) {
        StringBuilder sb = new StringBuilder();
        parameters = new ArrayList<>();
        for (Part p : parts) {
            List<ASTNode> args = p.getArguments();
            sb.append(p.getName());
            if (!args.isEmpty()) {
                sb.append('(');
                sb.append(args.size());
                sb.append(')');
            }
            parameters.addAll(args);
            for (ASTNode arg : args) {
                if (arg instanceof IdentifierDeclaration idDecl) {
                    parameterNames.add(idDecl.getName());
                }
            }
        }
        for (ASTNode member : body) {
            switch (member) {
                case VarDec varDec -> {
                    varNames.add(varDec.getName());
                }
                case DefDec defDec -> {
                    defNames.add(defDec.getName());
                }
                default -> {}
            }
        }
        this.name = sb.toString();
        this.returnType = returnType;
        this.annotations = annotations;
        this.body = body;
    }

    public String getName() {
        return name;
    }

    public List<? extends ASTNode> getBody() {
        return body;
    }

    public List<ASTNode> getParameters() {
        return parameters;
    }

    public ASTNode getReturnType() {
        return returnType;
    }

    public List<ASTNode> getAnnotations() {
        return annotations;
    }

    public List<String> getParameterNames() {
        return parameterNames;
    }

    public List<String> getVarNames() {
        return varNames;
    }

    public List<String> getDefNames() {
        return defNames;
    }

    public CPS bodyCPS() {
        CPS blockBody = (reqCtx, reqCont) -> {
            Context evalCtx = reqCtx.withReturnContinuation(reqCont);
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
        return blockBody;
    }

    public CPS toCPS() {
        return (_, cont) -> cont.apply(GraceObject.DONE);
    }
}
