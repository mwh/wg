package nz.mwh.wg.runtime;

import java.util.List;

import nz.mwh.wg.ast.*;

public class GraceBlock implements GraceObject {
    private GraceObject lexicalParent;
    private List<ASTNode> parameters;
    private List<ASTNode> body;
    private boolean matchingBlock = false;

    public GraceBlock(GraceObject lexicalParent, List<ASTNode> parameters, List<ASTNode> body) {
        this.lexicalParent = lexicalParent;
        this.parameters = parameters;
        this.body = body;
        if (parameters.size() == 1) {
            matchingBlock = true;
        }
    }

    @Override
    public GraceObject request(Request request) {
        if (request.parts.size() == 1) {
            if (request.parts.get(0).getName().equals("apply")) {
                return apply(request, request.parts.get(0));
            }
            if (matchingBlock && request.getName().equals("match(1)")) {
                return match(request, request.parts.get(0));
            }
            if (matchingBlock && request.getName().equals("|(1)")) {
                return new GracePatternOr(this, request.parts.get(0).getArgs().get(0));
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in Block(" + parameters.size() + "): " + request.getName());
    }

    private GraceObject apply(Request request, RequestPartR part) {
        BaseObject blockContext = new BaseObject(lexicalParent);
        for (int i = 0; i < parameters.size(); i++) {
            ASTNode parameter = parameters.get(i);
            String name;
            if (parameter instanceof IdentifierDeclaration) {
                name = ((IdentifierDeclaration) parameter).getName();
            } else if (parameter instanceof LexicalRequest) {
                name = ((LexicalRequest) parameter).getParts().get(0).getName();
            } else if (matchingBlock) {
                break;
            } else {
                throw new GraceException(request.getVisitor(), "Invalid parameter in block: " + parameter);
            }
            blockContext.addField(name);
            blockContext.setField(name, part.getArgs().get(i));
        }
        for (ASTNode stmt : body) {
            if (stmt instanceof DefDecl) {
                DefDecl def = (DefDecl) stmt;
                blockContext.addField(def.getName());
            } else if (stmt instanceof VarDecl) {
                VarDecl var = (VarDecl) stmt;
                blockContext.addField(var.getName());
                blockContext.addFieldWriter(var.getName());
            }
        }
        GraceObject last = null;
        for (ASTNode node : body) {
            last = node.accept(blockContext, request.getVisitor());
        }
        return last;
    }

    private GraceObject match(Request request, RequestPartR part) {
        ASTNode firstParameter = parameters.get(0);
        ASTNode patternExpr;
        if (firstParameter instanceof IdentifierDeclaration id) {
            patternExpr = id.getType();
        } else if (firstParameter instanceof NumberNode || firstParameter instanceof StringNode) {
            patternExpr = firstParameter;
        } else if (firstParameter instanceof ExplicitRequest er) {
            patternExpr = er;
        } else {
            throw new GraceException(request.getVisitor(), "Invalid parameter in matching block: " + firstParameter);
        }
        if (patternExpr == null) {
            GraceObject result = apply(request, part);
            return new GraceMatchResult(true, result);
        }
        GraceObject pattern = patternExpr.accept(lexicalParent, request.getVisitor());

        GraceObject target = part.getArgs().get(0);

        Request matchRequest = new Request(request.getVisitor(), List.of(new RequestPartR("match", List.of(target))));
        
        GraceObject matchResult = pattern.request(matchRequest);

        GraceObject matchSuccess = matchResult.request(new Request(request.getVisitor(), List.of(new RequestPartR("succeeded", List.of()))));
        if (matchSuccess instanceof GraceBoolean leftBool && leftBool.getValue()) {
            GraceObject result = apply(request, part);
            return new GraceMatchResult(true, result);
        }
        
        return matchResult;
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }
}

