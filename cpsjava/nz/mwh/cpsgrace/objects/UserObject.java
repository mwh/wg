package nz.mwh.cpsgrace.objects;

import java.util.List;
import java.util.Map;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class UserObject implements GraceObject {

    private Map<String, Method> methods;
    private UserObject surrounding;

    private String debugLabel;

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, List<GraceObject> args) {
        switch (methodName) {
            default:
                Method method = methods.get(methodName);
                if (method != null) {
                    return method.invoke(ctx.withSelfScope(this), returnCont, this, args);
                }
                System.out.println("no such method " + methodName + ": methods are " + getMethodNames());
                throw new RuntimeException("no such method " + methodName);
        }
    }

    public void addMethod(String name, Method method) {
        if (methods == null) {
            methods = new java.util.HashMap<>();
        }
        methods.put(name, method);
    }

    public void addVar(String name) {
        GraceObject[] value = new GraceObject[1];
        Method getter = Method.java((ctx, cont, _, _) -> {
            return cont.returning(ctx, value[0]);
        });
        Method setter = Method.java((ctx, cont, _, args) -> {
            GraceObject newValue = args.get(0);
            value[0] = newValue;
            return cont.returning(ctx, GraceObject.DONE);
        });
        addMethod(name, getter);
        addMethod(name + ":=(1)", setter);
    }

    public void addDef(String name) {
        GraceObject[] value = new GraceObject[1];
        Method getter = Method.java((ctx, cont, _, _) -> {
            return cont.returning(ctx, value[0]);
        });
        Method setter = Method.java((ctx, cont, _, args) -> {
            GraceObject newValue = args.get(0);
            value[0] = newValue;
            return cont.returning(ctx, GraceObject.DONE);
        });
        // Unutterable name
        addMethod(name + " =(1)", setter);
        addMethod(name, getter);
    }

    public boolean hasMethod(String name) {
        return methods != null && methods.containsKey(name);
    }
    
    public List<String> getMethodNames() {
        if (methods == null) {
            return java.util.Collections.emptyList();
        }
        return new java.util.ArrayList<>(methods.keySet());
    }

    public void setSurrounding(UserObject scope) {
        this.surrounding = scope;
    }

    public UserObject getSurrounding() {
        return surrounding;
    }

    public void setDebugLabel(String label) {
        this.debugLabel = label;
    }

    public String toString() {
        if (debugLabel != null) {
            return "<UserObject " + debugLabel + ">";
        } else {
            return "<UserObject>";
        }
    }
}
