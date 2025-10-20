package nz.mwh.wg;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Deque;
import java.util.stream.Collectors;

import nz.mwh.wg.ast.*;

import nz.mwh.wg.runtime.*;

public class Evaluator extends ASTConstructors implements Visitor<GraceObject> {

    private static GraceDone done = GraceDone.done;

    private Map<String, GraceObject> modules = new HashMap<>();

    private Deque<String> callStack = new ArrayDeque<>();

    @Override
    public GraceObject visit(GraceObject context, ObjectConstructor node) {
        BaseObject object = new BaseObject(context, false, true);
        List<ASTNode> body = node.getBody();
        for (ASTNode part : body) {
            if (part instanceof TypeDecl type) {
                object.addField(type.getName());
                object.setField(type.getName(), new GraceTypeReference(type.getName(), null));
            }
        }
        for (ASTNode part : body) {
            if (part instanceof DefDecl) {
                DefDecl def = (DefDecl) part;
                object.addField(def.getName());
            } else if (part instanceof VarDecl) {
                VarDecl var = (VarDecl) part;
                object.addField(var.getName());
                object.addFieldWriter(var.getName());
            } else if (part instanceof ImportStmt) {
                ImportStmt imp = (ImportStmt) part;
                object.addField(imp.getName());
            } else if (part instanceof MethodDecl) {
                visit(object, part);
            } else if (part instanceof TypeDecl type) {
                GraceObject actual = type.getType().accept(object, this);
                GraceObject ref = object.getField(type.getName());
                if (ref instanceof GraceTypeReference typeRef) {
                    typeRef.setType(actual);
                } else {
                    object.setField(type.getName(), actual);
                }
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
        for (Part part : node.getParts()) {
            List<GraceObject> args = part.getArgs().stream().map(x -> visit(context, x)).collect(Collectors.toList());
            parts.add(new RequestPartR(part.getName(), args));
        }
        Request request = new Request(this, parts, node.getLocation());
        int top = callStack.size();
        callStack.push(request.getName() + " at " + request.getLocation());
        GraceObject receiver = context.findReceiver(request.getName());
        if (receiver == null) {
            throw new GraceException(this, "No such method or variable in scope: " + request.getName());
        }
        GraceObject ret = receiver.request(request);
        while (callStack.size() > top) {
            callStack.pop();
        }
        return ret;
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
    public GraceObject visit(GraceObject context, InterpString node) {
        String value = node.getValue();
        ASTNode expression = node.getExpression();
        ASTNode next = node.getNext();
        StringBuilder sb = new StringBuilder();
        sb.append(value);
        sb.append(expression.accept(context, this).toString());
        while (next instanceof InterpString) {
            InterpString nextString = (InterpString) next;
            sb.append(nextString.getValue());
            expression = nextString.getExpression();
            next = nextString.getNext();
            sb.append(expression.accept(context, this).toString());
        }
        // next must now be a StringNode
        if (!(next instanceof StringNode)) {
            throw new UnsupportedOperationException("Invalid InterpString node");
        }
        StringNode sn = (StringNode) next;
        sb.append(sn.getValue());

        return new GraceString(sb.toString());
    }

    @Override
    public GraceObject visit(GraceObject context, DefDecl node) {
        if (context instanceof BaseObject) {
            BaseObject object = (BaseObject) context;
            GraceObject value = node.getValue().accept(context, this);
            object.setField(node.getName(), value);
            return done;
        }
        throw new GraceException(this, "def can only appear inside in-code context");
    }

    @Override
    public GraceObject visit(GraceObject context, VarDecl node) {
        if (node.getValue() != null) {
            new LexicalRequest(node.getLocation(), new Cons<Part>(new Part(node.getName() + ":=", new Cons<ASTNode>(node.getValue(), Cons.<ASTNode>nil() )), Cons.<Part>nil())).accept(context, this);
        }
        return done;
    }

    @Override
    public GraceObject visit(GraceObject context, MethodDecl node) {
        List<? extends Part> parts = node.getParts();
        String name = parts.stream().map(x -> x.getName() + "(" + x.getParameters().size() + ")").collect(Collectors.joining(""));
        if (context instanceof BaseObject) {
            BaseObject object = (BaseObject) context;
            List<? extends ASTNode> body = node.getBody();
            object.addMethod(name, request -> {
                BaseObject methodContext = new BaseObject(context, true);
                List<RequestPartR> requestParts = request.getParts();
                for (int j = 0; j < requestParts.size(); j++) {
                    Part part = parts.get(j);
                    RequestPartR rpart = requestParts.get(j);
                    List<? extends ASTNode> parameters = part.getParameters();
                    for (int i = 0; i < parameters.size(); i++) {
                        IdentifierDeclaration parameter = (IdentifierDeclaration) parameters.get(i);
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
                int top = callStack.size();
                try {
                    GraceObject last = null;
                    for (ASTNode part : body) {
                        last = visit(methodContext, part);
                    }
                    return last;
                } catch(ReturnException re) {
                    while (callStack.size() > top) {
                        callStack.pop();
                    }
                    if (re.context == methodContext) {
                        return re.getValue();
                    } else {
                        throw re;
                    }
                }
            });
            return done;
        }
        throw new GraceException(this, "method can only be defined in object context");
    }

    @Override
    public GraceObject visit(GraceObject context, ExplicitRequest node) {
        List<RequestPartR> parts = new ArrayList<>();
        for (Part part : node.getParts()) {
            List<GraceObject> args = part.getArgs().stream().map(x -> visit(context, x)).collect(Collectors.toList());
            parts.add(new RequestPartR(part.getName(), args));
        }
        Request request = new Request(this, parts, node.getLocation());
        GraceObject receiver = node.getReceiver().accept(context, this);
        int top = callStack.size();
        callStack.push(request.getName() + " at " + request.getLocation());
        GraceObject ret = receiver.request(request);
        while (callStack.size() > top) {
            callStack.pop();
        }
        return ret;
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
            if (receiver == null) {
                throw new GraceException(this, "No such method or variable in scope: " + request.getName());
            }    
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
        throw new GraceException(this, "Invalid assignment to non-lvalue " + node.getTarget().getClass().getName());
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
        throw new GraceException(this, "return can only appear inside method body");
    }

    @Override
    public GraceObject visit(GraceObject context, Comment node) {
        return done;
    }

    @Override
    public GraceObject visit(GraceObject context, ImportStmt node) {
        if (context instanceof BaseObject) {
            BaseObject object = (BaseObject) context;

            if (modules.containsKey(node.getSource())) {
                object.setField(node.getName(), modules.get(node.getSource()));
                return done;
            }

            String filename = node.getSource() + ".grace";
            try {
                String source = Files.readString(Path.of(filename));
                ObjectConstructor ast = (ObjectConstructor) Parser.parse(node.getSource(), source);
                GraceObject mod =  this.evaluateModule(ast);
                modules.put(node.getSource(), mod);
                object.setField(node.getName(), mod);
                return done;
            } catch (IOException e) {
                throw new GraceException(this, "Error reading file for import: " + filename);
            }
        }
                  
        throw new GraceException(this, "imports can only appear inside in-code context");
    }

    @Override
    public GraceObject visit(GraceObject context, DialectStmt node) {
        if (context instanceof BaseObject object) {

            if (modules.containsKey(node.getSource())) {
                object.setDialect(modules.get(node.getSource()));
                return done;
            }

            String filename = node.getSource() + ".grace";
            try {
                String source = Files.readString(Path.of(filename));
                ObjectConstructor ast = (ObjectConstructor) Parser.parse(node.getSource(), source);
                GraceObject mod =  this.evaluateModule(ast);
                modules.put(node.getSource(), mod);
                object.setDialect(mod);
                return done;
            } catch (IOException e) {
                throw new GraceException(this, "Error reading file for dialect import: " + filename);
            }
        }
        throw new GraceException(this, "dialect statements can only appear inside in-code context");
    }

    @Override
    public GraceObject visit(GraceObject context, InterfaceConstructor node) {
        List<GraceMethodSignature> methods = new ArrayList<>();
        for (MethodSignature meth : node.getBody()) {
            List<RequestPartR> parts = new ArrayList<>();
            for (var part : meth.getParts()) {
                List<GraceObject> args = part.getArgs().stream().map(x -> {
                    if (x instanceof IdentifierDeclaration id) {
                        if (id.getType() != null) {
                            return new GraceParameter(id.getName(), id.getType().accept(context, this));
                        } else {
                            return new GraceParameter(id.getName(), null);
                        }
                    }
                    throw new GraceException(this, "Invalid part in method signature: " + x.getClass().getName());
                }).collect(Collectors.toList());
                parts.add(new RequestPartR(part.getName(), args));
            }
            GraceObject returnType = null;
            if (meth.getReturnType() != null) {
                returnType = meth.getReturnType().accept(context, this);
            }
            methods.add(new GraceMethodSignature(parts, returnType));
        }
        return new GraceType(methods);
    }

    @Override
    public GraceObject visit(GraceObject context, TypeDecl node) {
        return done;
    }

    static BaseObject basePrelude() {
        BaseObject lexicalParent = new BaseObject(null);
        lexicalParent.addMethod("print(1)", request -> {
            System.out.println(request.getParts().get(0).getArgs().get(0).toString());
            return done;
        });
        lexicalParent.addMethod("true(0)", _ -> new GraceBoolean(true));
        lexicalParent.addMethod("false(0)", _ -> new GraceBoolean(false));
        lexicalParent.addMethod("if(1)then(1)else(1)", request -> {
            GraceObject condition = request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = List.of(new RequestPartR("ifTrue", List.of(request.getParts().get(1).getArgs().get(0))),
                                               new RequestPartR("ifFalse", List.of(request.getParts().get(2).getArgs().get(0))));
            Request req = new Request(request.getVisitor(), parts);
            return condition.request(req);
        });
        lexicalParent.addMethod("if(1)then(1)", request -> {
            GraceObject condition = request.getParts().get(0).getArgs().get(0);
            List<RequestPartR> parts = Collections.singletonList(new RequestPartR("ifTrue", List.of(request.getParts().get(1).getArgs().get(0))));
            Request req = new Request(request.getVisitor(), parts);
            condition.request(req);
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
        lexicalParent.addMethod("for(1)do(1)", request -> {
            GraceObject iterable = request.getParts().get(0).getArgs().get(0);
            GraceObject block = request.getParts().get(1).getArgs().get(0);
            Request eachReq = Request.unary(request.getVisitor(), "each", block);
            iterable.request(eachReq);
            return done;
        });
        String matchCase = "match(1)";
        for (int i = 0; i < 30; i++) {
            matchCase += "case(1)";
            lexicalParent.addMethod(matchCase, request -> {
                GraceObject target = request.getParts().get(0).getArgs().get(0);
                GraceObject pattern = request.getParts().get(1).getArgs().get(0);
                for (int j = 2; j < request.getParts().size(); j++) {
                    pattern = new GracePatternOr(pattern, request.getParts().get(j).getArgs().get(0));
                }
                return pattern.request(new Request(request.getVisitor(), List.of(new RequestPartR("match", List.of(target)))));
            });
        }
        String tryCatch = "try(1)";
        for (int i = 0; i < 10; i++) {
            tryCatch += "catch(1)";
            lexicalParent.addMethod(tryCatch, request -> {
                GraceObject block = request.getParts().get(0).getArgs().get(0);
                try {
                    block.request(Request.nullary(request.getVisitor(), "apply"));
                } catch (GraceException e) {
                    GraceObject target = e;
                    GraceObject pattern = request.getParts().get(1).getArgs().get(0);
                    GraceObject mr  = pattern.request(Request.unary(request.getVisitor(), "match", target));
                    if (mr instanceof GraceMatchResult matchResult) {
                        if (matchResult.isSuccess()) {
                            return done;
                        }
                    }
                    for (int j = 2; j < request.getParts().size(); j++) {
                        pattern = request.getParts().get(j).getArgs().get(0);
                        mr = pattern.request(Request.unary(request.getVisitor(), "match", target));
                        if (mr instanceof GraceMatchResult matchResult2) {
                            if (matchResult2.isSuccess()) {
                                return done;
                            }
                        }
                    }
                    throw e;
                }
                return done;
            });
        }
        lexicalParent.addMethod("Exception(0)", _ -> {
            return GraceExceptionKind.BASE;
        });
        lexicalParent.addMethod("getFileContents(1)", request -> {
            String filename = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            try {
                return new GraceString(new String(Files.readAllBytes(Paths.get(filename))));
            } catch (IOException e) {
                throw new GraceException(request.getVisitor(), "Error reading file: " + filename);
            }
        });
        return lexicalParent;
    }

    public Deque<String> getStack() {
        return new ArrayDeque<>(callStack);
    }

    public GraceObject evaluateModule(ObjectConstructor module) {
        return this.visit(basePrelude(), module);
    }

    public void bindModule(String name, GraceObject module) {
        modules.put(name, module);
    }

    static GraceObject evaluateProgram(ASTNode program) {
        BaseObject lexicalParent = (BaseObject) basePrelude();
        return evaluateProgram(program, lexicalParent);
    }

    static GraceObject evaluateProgram(ASTNode program, BaseObject lexicalParent) {
        Evaluator evaluator = new Evaluator();
        try {
            return evaluator.visit(lexicalParent, program);
        } catch (GraceException e) {
            throw e;
        } catch (RuntimeException e) {
            throw new GraceException(evaluator, "Java-level runtime error: " + e.getMessage()
                + " at " + String.join(" from ", Arrays.stream(e.getStackTrace()).map(x -> x.toString()).dropWhile(x -> x.startsWith("java.")).limit(2).collect(Collectors.joining(" from "))));
        }
    }

}
