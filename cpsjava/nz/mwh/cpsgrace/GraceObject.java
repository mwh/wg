package nz.mwh.cpsgrace;

public interface GraceObject {
    public static final GraceObject DONE = new GraceObject() {
        public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args) {
            throw new RuntimeException("Cannot request method on done");
        }
    };

    public static final GraceObject Uninitialised = new GraceObject() {
        public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args) {
            throw new RuntimeException("Cannot request method on uninitialised variable");
        }
    };
    
    public PendingStep requestMethod(Context ctx, Continuation returnCont, String methodName, java.util.List<GraceObject> args);
}
