package nz.mwh.wg.runtime;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

import nz.mwh.wg.Evaluator;

public class BaseObject implements GraceObject {
    private GraceObject lexicalParent;
    private GraceObject dialectParent = null;
    private Map<String, GraceObject> fields = new HashMap<>();
    private Map<String, Function<Request, GraceObject>> methods = new HashMap<>();

    protected static GraceDone done = GraceDone.done;
    protected static GraceUninitialised uninitialised = GraceUninitialised.uninitialised;

    private boolean returns = false;
    private boolean hasSelf = false;
    private boolean stateless = true;

    public BaseObject(GraceObject lexicalParent) {
        this(lexicalParent, false);
    }

    public BaseObject(GraceObject lexicalParent, boolean returns) {
        this(lexicalParent, returns, false);
    }

    public BaseObject(GraceObject lexicalParent, boolean returns, boolean bindSelf) {
        this.lexicalParent = lexicalParent;
        this.returns = returns;
        if (bindSelf) {
            addMethod("==(1)", request -> {
                GraceObject other = request.getParts().get(0).getArgs().get(0);
                return new GraceBoolean(this == other);
            });
            addMethod("!=(1)", request -> {
                GraceObject other = request.getParts().get(0).getArgs().get(0);
                return new GraceBoolean(this != other);
            });
            addMethod("asString(0)", _ -> new GraceString("object {" + String.join("; ", methods.keySet()) + "}"));
            addMethod("asDebugString(0)", _ -> new GraceString("object {" + String.join("; ", methods.keySet()) + "}"));
            addMethod("hash(0)", _ -> new GraceNumber(Integer.toUnsignedLong(System.identityHashCode(this))));
            addMethod("::(1)", request -> new GraceBinding(this, request.getParts().get(0).getArgs().get(0)));
            hasSelf = true;
            //addMethod("self(0)", _ -> this);
        }
    }

    public boolean hasSelf() {
        return hasSelf;
    }

    public String toString() {
        if (!hasSelf) {
            return "scope {" + String.join("; ", methods.keySet()) + "}" + " within " + lexicalParent;
        }
        var eval = new Evaluator();
        eval.setSelf(this);
        Request request = new Request(eval, Collections.singletonList(new RequestPartR("asString", Collections.emptyList())));
        return request(request).toString();
    }

    public void addMethod(String name, Function<Request, GraceObject> method) {
        methods.put(name, method);
    }

    @Override
    public GraceObject request(Request request) {
        Function<Request, GraceObject> method = methods.get(request.getName());
        if (method != null) {
            var oldSelf = request.getVisitor().getSelf();
            request.getVisitor().setSelf(this);
            GraceObject result = method.apply(request);
            request.getVisitor().setSelf(oldSelf);
            return result;
        }
        if (fields.containsKey(request.getName())) {
            return fields.get(request.getName());
        }
        if (request.getParts().size() == 1 && request.getParts().get(0).getName().endsWith(":=(1)")) {
            RequestPartR part = request.getParts().get(0);
            fields.put(part.getName().substring(0, part.getName().length() - 5), part.getArgs().get(0));
            return done;
        }
        throw new GraceException(request.getVisitor(), "No such method or field " + request.getName() + " in object with methods " + methods.keySet());
    }

    public GraceObject findReceiver(String name) {
        if (methods.containsKey(name) || fields.containsKey(name)) {
            return this;
        }
        if (dialectParent != null && dialectParent.hasMethod(name)) {
            return dialectParent;
        }
        if (lexicalParent != null) {
            return lexicalParent.findReceiver(name);
        }
        return null;
    }

    public void addField(String name) {
        fields.put(name, uninitialised);
        methods.put(name + "(0)", request -> {
            GraceObject val = fields.get(name);
            if (val == uninitialised) {
                throw new GraceException(request.getVisitor(), "Field " + name + " is not initialised; other fields are " + fields.keySet());
            }
            return val;
        });
    }

    public void addFieldWriter(String name) {
        methods.put(name + ":=(1)", request -> {
            fields.put(name, request.getParts().get(0).getArgs().get(0));
            return done;
        });
        stateless = false;
    }

    public void setField(String name, GraceObject value) {
        fields.put(name, value);
    }

    public GraceObject getField(String name) {
        GraceObject value = fields.get(name);
        return value;
    }

    public GraceObject findReturnContext() {
        if (returns) {
            return this;
        }
        if (lexicalParent != null) {
            return ((BaseObject) lexicalParent).findReturnContext();
        }
        throw new RuntimeException("No return context found");
    }

    public GraceObject findNearestSelf() {
        if (hasSelf) {
            return this;
        }
        if (lexicalParent != null) {
            return ((BaseObject) lexicalParent).findNearestSelf();
        }
        return null;
    }

    public void useObject(BaseObject mixin) {
        for (Map.Entry<String, Function<Request, GraceObject>> method : mixin.methods.entrySet()) {
            switch(method.getKey()) {
                case "==(1)", "!=(1)", "asString(0)", "asDebugString(0)", "hash(0)", "::(1)", "self(0)" -> {
                    // skip these methods
                }
                default -> methods.put(method.getKey(), method.getValue());
            }
        }
        for (Map.Entry<String, GraceObject> field : mixin.fields.entrySet()) {
            fields.put(field.getKey(), field.getValue());
        }
    }

    public boolean isStateless() {
        return stateless;
    }


    @Override
    public boolean hasMethod(String name) {
        return methods.containsKey(name);
    }

    public void setDialect(GraceObject dialect) {
        dialectParent = dialect;
    }

}

