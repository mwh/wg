package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

public class Method {
    private List<String> parameterNames;
    private List<String> genericParameterNames;
    private List<String> vars;
    private List<String> defs;
    private CPS body;
    private boolean captureReturn = true;

    private String debugDescription;

    protected Method() {
        // For subclassing
    }

    public Method(CPS body) {
        this.parameterNames = List.of();
        this.body = body;
        this.vars = List.of();
        this.defs = List.of();
    }

    public Method(List<String> parameterNames, List<String> genericParameters, CPS body, List<String> vars, List<String> defs, boolean captureReturn) {
        // Store the parameter names and body for later use
        this.parameterNames = parameterNames;
        this.genericParameterNames = genericParameters;
        this.body = body;
        this.vars = vars;
        this.defs = defs;
        this.captureReturn = captureReturn;
    }

    public Method described(String description) {
        this.debugDescription = description;
        return this;
    }

    public String toString() {
        if (debugDescription != null) {
            return "<Method " + debugDescription + ">";
        } else {
            return "<Method>";
        }
    }

    public PendingStep invoke(Context ctx, Continuation returnCont, GraceObject self, List<GraceObject> args, List<GraceObject> genericArgs) {
        Context bodyContext = ctx.withSelf((UserObject) self);
        if (!captureReturn) {
            bodyContext = bodyContext.withReturnContinuation(returnCont);
        }
        UserObject localScope = bodyContext.extendScope(toString());
        for (int i = 0; i < parameterNames.size(); i++) {
            String paramName = parameterNames.get(i);
            GraceObject argValue = args.get(i);
            bodyContext.bindLocalName(paramName, argValue);
        }
        for (int i = 0; i < genericParameterNames.size(); i++) {
            String genName = genericParameterNames.get(i);
            if (i >= genericArgs.size()) {
                GraceObject argValue = new UserObject();
                bodyContext.bindLocalName(genName, argValue);
            } else {
                GraceObject argValue = genericArgs.get(i);
                bodyContext.bindLocalName(genName, argValue);
            }
        }
        for (String varName : vars) {
            localScope.addVar(varName);
        }
        for (String defName : defs) {
            localScope.addDef(defName);
        }
        // Implement the method body using bodyContext and args
        return body.run(bodyContext, returnCont);
    }

    public static NativeMethod java(NativeMethodFunction function) {
        return new NativeMethod(function);
    }

    public static class NativeMethod extends Method {
        private NativeMethodFunction function;

        public NativeMethod(NativeMethodFunction function) {
            this.function = function;
        }

        @Override
        public PendingStep invoke(Context ctx, Continuation returnCont, GraceObject self, List<GraceObject> args, List<GraceObject> genericArgs) {
            return function.apply(ctx, returnCont, self, args);
        }
    }

    public interface NativeMethodFunction {
        PendingStep apply(Context ctx, Continuation returnCont, GraceObject self, List<GraceObject> args);
    }
}
