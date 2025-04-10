package nz.mwh.wg.runtime;

import java.util.List;

import nz.mwh.wg.ast.*;

public class GraceBlock implements GraceObject {
    private GraceObject lexicalParent;
    private List<ASTNode> parameters;
    private List<ASTNode> body;

    public GraceBlock(GraceObject lexicalParent, List<ASTNode> parameters, List<ASTNode> body) {
        this.lexicalParent = lexicalParent;
        this.parameters = parameters;
        this.body = body;
    }

    @Override
    public GraceObject request(Request request) {
        if (request.parts.size() == 1) {
            if (request.parts.get(0).getName().equals("apply")) {
                return apply(request, request.parts.get(0));
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in Block(" + parameters.size() + "): " + request.getName());
    }

    private GraceObject apply(Request request, RequestPartR part) {
        BaseObject blockContext = new BaseObject(lexicalParent);
        blockContext.incRefCount();
        for (int i = 0; i < parameters.size(); i++) {
            ASTNode parameter = parameters.get(i);
            String name;
            if (parameter instanceof IdentifierDeclaration) {
                name = ((IdentifierDeclaration) parameter).getName();
            } else if (parameter instanceof LexicalRequest) {
                name = ((LexicalRequest) parameter).getParts().get(0).getName();
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
            if (last != null) {
                last.discard();
            }
            last = node.accept(blockContext, request.getVisitor());
        }
        for (GraceObject field : blockContext.getFields().values()) {
            if (field == last) {
                last.incRefCount();
                last.beReturned();
                break;
            }
                
        }
        blockContext.decRefCount();
        return last;
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }
}

