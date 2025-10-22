package nz.mwh.cpsgrace;

public class PendingStep {
    private Context context;
    private Continuation continuation;
    private GraceObject argument;

    public PendingStep(Context ctx, Continuation cont, GraceObject arg) {
        this.context = ctx;
        this.continuation = cont;
        this.argument = arg;
    }

    public PendingStep go() {
        return continuation.apply(argument);
    }

    public Context getContext() {
        return context;
    }
    
}
