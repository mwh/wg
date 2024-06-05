package nz.mwh.wg;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.stream.Collectors;

import nz.mwh.wg.ast.*;

import nz.mwh.wg.runtime.*;

public class Evaluator extends ASTConstructors implements Visitor<GraceObject> {

    private static GraceDone done = GraceDone.done;

    @Override
    public GraceObject visit(GraceObject context, ObjectConstructor node) {
        BaseObject object = new BaseObject(context);
        List<ASTNode> body = node.getBody();
        for (ASTNode part : body) {
            if (part instanceof DefDecl) {
                DefDecl def = (DefDecl) part;
                object.addField(def.getName());
            } else if (part instanceof VarDecl) {
                VarDecl var = (VarDecl) part;
                object.addField(var.getName());
                object.addFieldWriter(var.getName());
            } else if (part instanceof MethodDecl) {
                visit(object, part);
            }
        }
        for (ASTNode part : body) {
            visit(object, part);
        }
        return object;
    }

    @Override
    public GraceObject visit(GraceObject context, LexicalRequest node) {
        List<RequestPartR> parts = new ArrayList<>();
        for (RequestPart part : node.getParts()) {
            List<GraceObject> args = part.getArgs().stream().map(x -> visit(context, x)).collect(Collectors.toList());
            parts.add(new RequestPartR(part.getName(), args));
        }
        Request request = new Request(this, parts);
        GraceObject receiver = context.findReceiver(request.getName());
        return receiver.request(request);
    }

    @Override
    public GraceObject visit(GraceObject context, NumberNode node) {
        return new GraceNumber(node.getValue());
    }

    @Override
    public GraceObject visit(GraceObject context, StringNode node) {
        return new GraceString(node.getValue());
    }

    @Override
    public GraceObject visit(GraceObject context, DefDecl node) {
        if (context instanceof BaseObject) {
            BaseObject object = (BaseObject) context;
            GraceObject value = node.getValue().accept(context, this);
            object.setField(node.getName(), value);
            return done;
        }
        throw new UnsupportedOperationException("def can only appear inside in-code context");
    }

    @Override
    public GraceObject visit(GraceObject context, VarDecl node) {
        if (node.getValue() != null) {
            new LexicalRequest(new Cons<RequestPart>(new RequestPart(node.getName() + ":=", new Cons<ASTNode>(node.getValue(), Cons.<ASTNode>nil() )), Cons.<RequestPart>nil())).accept(context, this);
        }
        return done;
    }

    @Override
    public GraceObject visit(GraceObject context, MethodDecl node) {
        List<? extends DeclarationPart> parts = node.getParts();
        String name = parts.stream().map(x -> x.getName() + "(" + x.getParameters().size() + ")").collect(Collectors.joining(""));
        if (context instanceof BaseObject) {
            BaseObject object = (BaseObject) context;
            List<? extends ASTNode> body = node.getBody();
            object.addMethod(name, request -> {
                BaseObject methodContext = new BaseObject(context, true);
                List<RequestPartR> requestParts = request.getParts();
                for (int j = 0; j < requestParts.size(); j++) {
                    DeclarationPart part = parts.get(j);
                    RequestPartR rpart = requestParts.get(j);
                    List<? extends IdentifierDeclaration> parameters = part.getParameters();
                    for (int i = 0; i < parameters.size(); i++) {
                        IdentifierDeclaration parameter = parameters.get(i);
                        methodContext.addField(parameter.getName());
                        methodContext.setField(parameter.getName(), rpart.getArgs().get(i));
                    }
                }
                for (ASTNode part : body) {
                    if (part instanceof DefDecl) {
                        DefDecl def = (DefDecl) part;
                        methodContext.addField(def.getName());
                    } else if (part instanceof VarDecl) {
                        VarDecl var = (VarDecl) part;
                        methodContext.addField(var.getName());
                        methodContext.addFieldWriter(var.getName());
                    }
                }        
                try {
                    GraceObject last = null;
                    for (ASTNode part : body) {
                        last = visit(methodContext, part);
                    }
                    return last;
                } catch(ReturnException re) {
                    if (re.context == methodContext) {
                        return re.getValue();
                    } else {
                        throw re;
                    }
                }
            });
            return done;
        }
        throw new UnsupportedOperationException("method can only be defined in object context");
    }

    @Override
    public GraceObject visit(GraceObject context, ExplicitRequest node) {
        List<RequestPartR> parts = new ArrayList<>();
        for (RequestPart part : node.getParts()) {
            List<GraceObject> args = part.getArgs().stream().map(x -> visit(context, x)).collect(Collectors.toList());
            parts.add(new RequestPartR(part.getName(), args));
        }
        Request request = new Request(this, parts, node.location);
        GraceObject receiver = node.getReceiver().accept(context, this);
        return receiver.request(request);

    }

    @Override
    public GraceObject visit(GraceObject context, Assign node) {
        if (node.getTarget() instanceof LexicalRequest) {
            LexicalRequest target = (LexicalRequest) node.getTarget();
            String name = target.getParts().get(0).getName();
            List<RequestPartR> parts = new ArrayList<>();
            parts.add(new RequestPartR(name + ":=", Collections.singletonList(node.getValue().accept(context, this))));
            Request request = new Request(this, parts);
            GraceObject receiver = context.findReceiver(request.getName());
            receiver.request(request);
            return done;
        } else if (node.getTarget() instanceof ExplicitRequest) {
            ExplicitRequest target = (ExplicitRequest) node.getTarget();
            String name = target.getParts().get(0).getName();
            List<RequestPartR> parts = new ArrayList<>();
            parts.add(new RequestPartR(name + ":=", Collections.singletonList(node.getValue().accept(context, this))));
            Request request = new Request(this, parts);
            GraceObject receiver = target.getReceiver().accept(context, this);
            receiver.request(request);
            return done;
        }
        throw new UnsupportedOperationException("Invalid assignment to " + node.getTarget().getClass().getName() + " node");
    }

    @Override
    public GraceObject visit(GraceObject context, Block node) {
        List<ASTNode> parameters = node.getParameters();
        List<ASTNode> body = node.getBody();

        return new GraceBlock(context, parameters, body);
    }

    @Override
    public GraceObject visit(GraceObject context, ReturnStmt node) {
        GraceObject value = visit(context, node.getValue());
        if (context instanceof BaseObject) {
            GraceObject returnContext = ((BaseObject)context).findReturnContext();
            throw new ReturnException(returnContext, value);
        }
        throw new UnsupportedOperationException("return can only appear inside method body");
    }

    @Override
    public GraceObject visit(GraceObject context, Comment node) {
        return done;
    }

    static BaseObject basePrelude() {
        BaseObject lexicalParent = new BaseObject(null);
        lexicalParent.addMethod("print(1)", request -> {
            System.out.println(request.getParts().get(0).getArgs().get(0).toString());
            return done;
        });
        lexicalParent.addMethod("true(0)", request -> new GraceBoolean(true));
        lexicalParent.addMethod("false(0)", request -> new GraceBoolean(false));
        lexicalParent.addMethod("if(1)then(1)else(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            } else {
                return request.getParts().get(2).getArgs().get(0).request(req);
            }
        });
        lexicalParent.addMethod("if(1)then(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            }
            return done;
        });
        lexicalParent.addMethod("if(1)then(1)elseif(1)then(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            List<RequestPartR> rparts = request.getParts();
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(2).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(3).getArgs().get(0).request(req);
            } else {
                return done;
            }
        });
        lexicalParent.addMethod("if(1)then(1)elseif(1)then(1)else(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            List<RequestPartR> rparts = request.getParts();
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(2).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(3).getArgs().get(0).request(req);
            } else {
                return request.getParts().get(4).getArgs().get(0).request(req);
            }
        });
        lexicalParent.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            List<RequestPartR> rparts = request.getParts();
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(2).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(3).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(4).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(5).getArgs().get(0).request(req);
            } else {
                return request.getParts().get(6).getArgs().get(0).request(req);
            }
        });
        lexicalParent.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            List<RequestPartR> rparts = request.getParts();
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(2).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(3).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(4).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(5).getArgs().get(0).request(req);
            } else {
                return done;
            }
        });
        lexicalParent.addMethod("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", request -> {
            GraceBoolean condition = (GraceBoolean) request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            List<RequestPartR> rparts = request.getParts();
            if (condition.getValue()) {
                return request.getParts().get(1).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(2).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(3).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(4).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(5).getArgs().get(0).request(req);
            } else if (((GraceBoolean)rparts.get(6).getArgs().get(0).request(req)).getValue()) {
                return rparts.get(7).getArgs().get(0).request(req);
            } else {
                return request.getParts().get(8).getArgs().get(0).request(req);
            }
        });
        lexicalParent.addMethod("while(1)do(1)", request -> {
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("apply", Collections.emptyList()));
            Request req = new Request(request.getVisitor(), parts);
            GraceObject condition = request.getParts().get(0).getArgs().get(0);
            GraceObject body = request.getParts().get(1).getArgs().get(0);
            while (((GraceBoolean) condition.request(req)).getValue()) {
                body.request(req);
            }
            return done;
        });
        lexicalParent.addMethod("getFileContents(1)", request -> {
            String filename = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            try {
                return new GraceString(new String(Files.readAllBytes(Paths.get(filename))));
            } catch (IOException e) {
                throw new RuntimeException("Error reading file: " + filename);
            }
        });
        return lexicalParent;
    }

    static GraceObject evaluateProgram(ASTNode program) {
        BaseObject lexicalParent = (BaseObject) basePrelude();
        return evaluateProgram(program, lexicalParent);
    }

    static GraceObject evaluateProgram(ASTNode program, BaseObject lexicalParent) {
        Evaluator evaluator = new Evaluator();
        return evaluator.visit(lexicalParent, program);
    }


}
