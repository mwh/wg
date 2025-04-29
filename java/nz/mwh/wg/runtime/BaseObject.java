package nz.mwh.wg.runtime;

import java.util.Collections;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;

import nz.mwh.wg.Dala;
import nz.mwh.wg.Evaluator;
import nz.mwh.wg.Visitor;

public class BaseObject implements GraceObject {
    private GraceObject lexicalParent;
    private Map<String, GraceObject> fields = new HashMap<>();
    private Map<String, Function<Request, GraceObject>> methods = new HashMap<>();

    private int refCount = 0;
    private boolean notionalReference = false;

    protected static GraceDone done = GraceDone.done;
    protected static GraceUninitialised uninitialised = GraceUninitialised.uninitialised;

    private boolean returns = false;

    private Dala.Flavour flavour = Dala.Flavour.UNSAFE;

    public BaseObject(GraceObject lexicalParent) {
        this(lexicalParent, false);
    }

    public BaseObject(GraceObject lexicalParent, boolean returns) {
        this(lexicalParent, returns, false);
    }

    public BaseObject(GraceObject lexicalParent, boolean returns, boolean bindSelf) {
        this.lexicalParent = lexicalParent;
        this.returns = returns;
        addMethod("==(1)", request -> {
            GraceObject other = request.getParts().get(0).getArgs().get(0);
            return new GraceBoolean(this == other);
        });
        addMethod("!=(1)", request -> {
            GraceObject other = request.getParts().get(0).getArgs().get(0);
            return new GraceBoolean(this != other);
        });
        if (bindSelf) {
            addMethod("self(0)", _ -> this);
        }
    }

    public String toString() {
        Request request = new Request(new Evaluator(), Collections.singletonList(new RequestPartR("asString", Collections.emptyList())));
        return request(request).toString();
    }

    public void addMethod(String name, Function<Request, GraceObject> method) {
        methods.put(name, method);
    }

    @Override
    public GraceObject request(Request request) {
        Function<Request, GraceObject> method = methods.get(request.getName());
        if (isIso() && (Dala.getIsoWhen() == Dala.IsoWhen.DEREFERENCE || Dala.getIsoWhen() == Dala.IsoWhen.DEREFERENCE_THREAD) && refCount > 1) {
            throw new GraceException(request.getVisitor(), "iso object dereferenced with multiple references");
        }
        if (method != null) {
            return method.apply(request);
        }
        if (fields.containsKey(request.getName())) {
            return fields.get(request.getName());
        }
        if (request.getParts().size() == 1 && request.getParts().get(0).getName().endsWith(":=(1)")) {
            RequestPartR part = request.getParts().get(0);
            fields.put(part.getName().substring(0, part.getName().length() - 5), part.getArgs().get(0));
            return done;
        }
        throw new GraceException(request.getVisitor(), "No such method or field " + request.getName() + " in object");
    }

    public GraceObject findReceiver(String name) {
        //System.out.println("searching for receiver for " + name + ", this object has methods " + methods.keySet());
        if (methods.containsKey(name) || fields.containsKey(name)) {
            return this;
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
            GraceObject old = fields.get(name);
            GraceObject newVal = request.getParts().get(0).getArgs().get(0);
            setField(request.getVisitor(), name, newVal);
            // Do not decrement the old value's count, as it is switched into a notional reference
            // during the return.
            return old.beReturned();
        });
    }

    public void setField(Visitor<GraceObject> visitor, String name, GraceObject value) {
        fields.put(name, value);
        value.incRefCount();
        switch (value) {
            case BaseObject o:
                if (o.isIso() && (Dala.getIsoWhen() == Dala.IsoWhen.ASSIGNMENT) && (o.getRefCount() > 1)) {
                    throw new GraceException(visitor, "illegal alias created to iso object");
                }
                break;
            default:
        }
    }

    public void setField(String name, GraceObject value) {
        fields.put(name, value);
        value.incRefCount();
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

    public GraceObject beReturned() {
        notionalReference = true;
        return this;
    }

    public boolean isBeingReturned() {
        return notionalReference;
    }

    public void incRefCount() {
        if (notionalReference) {
            // If there is a notional reference (in the midst of being returned),
            // we just take over that reference as real.
            notionalReference = false;
        } else {
            refCount++;
        }
    }

    public void decRefCount() {
        refCount--;
        if (refCount == 0) {
            free();
        }
    }

    public void discard() {
        if (notionalReference) {
            notionalReference = false;
            decRefCount();
        }
    }

    private void free() {
        for (GraceObject field : fields.values()) {
            field.decRefCount();
        }
    }

    public int getRefCount() {
        return refCount;
    }

    public Map<String, GraceObject> getFields() {
        return fields;
    }

    public boolean isIso() {
        return flavour == Dala.Flavour.ISO;
    }

    public boolean isUnsafe() {
        return flavour == Dala.Flavour.UNSAFE;
    }

    public boolean isLocal() {
        return flavour == Dala.Flavour.LOCAL;
    }

    public boolean isImm() {
        return flavour == Dala.Flavour.IMM;
    }

    public void setFlavour(Dala.Flavour flavour) {
        this.flavour = flavour;
    }
}

