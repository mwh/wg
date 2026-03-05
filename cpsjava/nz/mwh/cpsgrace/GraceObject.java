package nz.mwh.cpsgrace;

public interface GraceObject {
    public static final GraceObject DONE = new GraceObject() {
        public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args, java.util.List<GraceObject> genericArgs) {
            throw new RuntimeException("Cannot request method on done");
        }
    };

    public static final GraceObject Uninitialised = new GraceObject() {
        public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args, java.util.List<GraceObject> genericArgs) {
            throw new RuntimeException("Cannot request method on uninitialised variable");
        }
    };

    public default PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args) {
        return requestMethod(ctx, returnCont, methodName, args, java.util.List.of());
    }

    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args, java.util.List<GraceObject> genericArgs);

    /** Returns true if this object responds to the given method name */
    public default boolean hasMethod(String name) {
        return false;
    }
}
