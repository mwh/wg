package nz.mwh.cpsgrace.objects;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.Continuation;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.PendingStep;

/**
 * A mutable reference to a type, used to support forward and circular type
 * references. Type declarations are pre-populated with an unresolved
 * GraceTypeRef, and the ref is filled in when the type expression is evaluated.
 * Other type expressions evaluated in the meantime will hold a reference to this
 * object, so they will see the resolved type once it is set.
 */
public class GraceTypeRef implements GraceObject {
    private final String name;
    private GraceObject type;

    public GraceTypeRef(String name) {
        this.name = name;
    }

    public void setType(GraceObject type) {
        this.type = type;
    }

    public GraceObject getType() {
        return type;
    }

    @Override
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName,
            List<GraceObject> args, List<GraceObject> genericArgs) {
        if (type != null) {
            return type.requestMethod(ctx, returnCont, methodName, args, genericArgs);
        }
        throw new RuntimeException("Type reference '" + name + "' is not yet initialised, cannot request: " + methodName);
    }

    @Override
    public String toString() {
        if (type == null) return "<uninitialised type " + name + ">";
        return type.toString();
    }
}
