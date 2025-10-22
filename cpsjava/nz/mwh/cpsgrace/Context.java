package nz.mwh.cpsgrace;

import nz.mwh.cpsgrace.objects.Method;
import nz.mwh.cpsgrace.objects.UserObject;

public class Context {
    
    private UserObject self;
    private UserObject scope;
    private Continuation returnContinuation;
    private Continuation exceptionContinuation;

    public Context() {
        this.self = null;
        this.returnContinuation = null;
        this.exceptionContinuation = null;
    }

    public Context(Context other) {
        this.self = other.self;
        this.returnContinuation = other.returnContinuation;
        this.exceptionContinuation = other.exceptionContinuation;
        this.scope = other.scope;
    }

    public Context withSelf(UserObject self) {
        Context newCtx = new Context(this);
        newCtx.self = self;
        return newCtx;
    }

    public Context withSelfScope(UserObject self) {
        Context newCtx = new Context(this);
        newCtx.self = self;
        newCtx.scope = self;
        return newCtx;
    }

    public Context withScope(UserObject scope) {
        Context newCtx = new Context(this);
        newCtx.scope = scope;
        return newCtx;
    }

    public GraceObject getSelf() {
        return self;
    }

    public UserObject extendScope() {
        return extendScope(null);
    }

    public UserObject extendScope(String label) {
        UserObject newScope = new UserObject();
        newScope.setDebugLabel(label);
        if (this.scope != null) {
            newScope.setSurrounding(this.scope);
        }
        this.scope = newScope;
        return newScope;
    }

    public void bindLocalName(String name, GraceObject value) {
        if (this.scope == null) {
            throw new RuntimeException("No scope to bind local name " + name);
        }
        this.scope.addMethod(name, new Method((_, cont) -> new PendingStep(this, cont, value)));
    }

    public UserObject getScope() {
        return scope;
    }

    public GraceObject findReceiver(String name) {
        UserObject current = scope;
        // System.err.println("\n\nLooking for receiver of method " + name);
        while (current != null) {
            // System.err.println("Checking scope: " + current);
            // System.err.println("  Methods: " + current.getMethodNames());
            if (current.hasMethod(name)) {
                return current;
            }
            current = current.getSurrounding();
        }
        System.err.println("No receiver found for method " + name);
        return null;
    }

    public Context withReturnContinuation(Continuation returnContinuation) {
        Context newCtx = new Context(this);
        newCtx.returnContinuation = returnContinuation;
        return newCtx;
    }

    public Continuation getReturnContinuation() {
        return returnContinuation;
    }

    public Context withExceptionContinuation(Continuation exceptionContinuation) {
        Context newCtx = new Context(this);
        newCtx.exceptionContinuation = exceptionContinuation;
        return newCtx;
    }
}
