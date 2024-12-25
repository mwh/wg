package nz.mwh.wg;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.stream.Collectors;

import nz.mwh.wg.ast.*;
import nz.mwh.wg.css.Rule;
import nz.mwh.wg.runtime.*;

public class Evaluator extends ASTConstructors implements Visitor<GraceObject> {

    private static GraceDone done = GraceDone.done;

    private Map<String, GraceObject> modules = new HashMap<>();

    private static List<nz.mwh.wg.css.Rule> cssRules = new ArrayList<>();
    private List<List<nz.mwh.wg.css.Rule>> activeRules = new ArrayList<>();
    private List<List<nz.mwh.wg.css.Rule>> frameRules = new ArrayList<>();

    public static void addRule(nz.mwh.wg.css.Rule rule) {
        cssRules.add(rule);
    }

    public Evaluator() {
        activeRules.add(cssRules);
        frameRules.add(new ArrayList<>());
    }

    @Override
    public GraceObject visit(GraceObject context, ObjectConstructor node) {
        BaseObject object = new BaseObject(context, false, true);
        List<Rule> newRules = new ArrayList<>();
        for (Rule r : activeRules.getLast()) {
            newRules.addAll(r.successors(node, context));
        }
        activeRules.add(newRules);
        List<ASTNode> body = node.getBody();
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
            }
        }
        for (ASTNode part : body) {
            visit(object, part);
        }
        activeRules.removeLast();
        return object;
    }

    @Override
    public GraceObject visit(GraceObject context, LexicalRequest node) {
        List<RequestPartR> parts = new ArrayList<>();
        for (Part part : node.getParts()) {
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

        return new GraceString(value + expression.accept(context, this) + next.accept(context, this));
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
        List<Rule> newRules = new ArrayList<>();
        for (Rule r : activeRules.getLast()) {
            // System.out.println("at vardecl " + node + " with " + r);
            newRules.addAll(r.successors(node, context));
        }

        activeRules.add(newRules);
        if (node.getValue() != null) {
            List<GraceObject> argList = Collections.singletonList(node.getValue().accept(context, this));
            String name = node.getName();
            List<RequestPartR> parts = new ArrayList<>();
            parts.add(new RequestPartR(name + ":=", argList));
            Request request = new Request(this, parts);
            for (Rule r : activeRules.getLast()) {
                r.execute(node, context, argList.get(0));
            }
            context.request(request);
            // new LexicalRequest(new Cons<Part>(new Part(node.getName() + ":=", new Cons<ASTNode>(node.getValue(), Cons.<ASTNode>nil() )), Cons.<Part>nil())).accept(context, this);
        }
        activeRules.removeLast();
        return done;
    }

    @Override
    public GraceObject visit(GraceObject context, MethodDecl node) {
        List<? extends Part> parts = node.getParts();
        String name = parts.stream().map(x -> x.getName() + "(" + x.getParameters().size() + ")").collect(Collectors.joining(""));
        List<Rule> newRules = new ArrayList<>();
        for (Rule r : activeRules.getLast()) {
            newRules.addAll(r.successors(node, context));
        }
        newRules.addAll(frameRules.getLast());
        if (context instanceof BaseObject) {
            BaseObject object = (BaseObject) context;
            List<? extends ASTNode> body = node.getBody();
            object.addMethod(name, request -> {
                List<Rule> dynRules = new ArrayList<>(newRules);
                Set<Rule> seen = new HashSet<>(dynRules);
                List<Rule> myFrameRules = new ArrayList<>();
                for (Rule r : newRules) {
                    List<Rule> succs = r.frameSuccessors(name, node, context);
                    for (Rule s : succs) {
                        if (!seen.contains(s)) {
                            seen.add(s);
                            myFrameRules.add(s);
                        }
                    }
                }
                for (Rule r : frameRules.getLast()) {
                    List<Rule> succs = r.frameSuccessors(name, node, context);
                    for (Rule s : succs) {
                        if (!seen.contains(s)) {
                            seen.add(s);
                            myFrameRules.add(s);
                        }
                    }
                }
                dynRules.addAll(myFrameRules);
                frameRules.add(myFrameRules);
                // System.out.println("method " + name + " called; frame rules: " + frameRules);
                // System.out.println("method " + name + " called; newrules: " + newRules);
                BaseObject methodContext = new BaseObject(context, true);
                final int baseRulesLength = activeRules.size();
                final int frameRulesLength = frameRules.size();
                activeRules.add(dynRules);
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
                try {
                    GraceObject last = null;
                    for (ASTNode part : body) {
                        last = visit(methodContext, part);
                    }
                    activeRules.removeLast();
                    while (activeRules.size() > baseRulesLength) {
                        activeRules.removeLast();
                    }
                    frameRules.removeLast();
                    while (frameRules.size() > frameRulesLength) {
                        frameRules.removeLast();
                    }
                    return last;
                } catch(ReturnException re) {
                    if (re.context == methodContext) {
                        activeRules.removeLast();
                        while (activeRules.size() > baseRulesLength) {
                            activeRules.removeLast();
                        }
                        frameRules.removeLast();
                        while (frameRules.size() > frameRulesLength) {
                            frameRules.removeLast();
                        }
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
        for (Part part : node.getParts()) {
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
            List<Rule> newRules = new ArrayList<>();
            for (Rule r : activeRules.getLast()) {
                newRules.addAll(r.successors(node, context));
            }
            activeRules.add(newRules);
            List<GraceObject> argList = Collections.singletonList(node.getValue().accept(context, this));
            for (Rule r : activeRules.getLast()) {
                r.execute(node, context, argList.get(0));
            }
            LexicalRequest target = (LexicalRequest) node.getTarget();
            String name = target.getParts().get(0).getName();
            List<RequestPartR> parts = new ArrayList<>();
            parts.add(new RequestPartR(name + ":=", argList));
            Request request = new Request(this, parts);
            GraceObject receiver = context.findReceiver(request.getName());
            receiver.request(request);
            activeRules.removeLast();
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
                ObjectConstructor ast = (ObjectConstructor) Parser.parse(source);
                GraceObject mod =  this.evaluateModule(ast);
                modules.put(node.getSource(), mod);
                object.setField(node.getName(), mod);
                return done;
            } catch (IOException e) {
                throw new RuntimeException("Error reading file: " + filename);
            }
        }
                  
        throw new UnsupportedOperationException("imports can only appear inside in-code context");
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
        return evaluator.visit(lexicalParent, program);
    }


}
