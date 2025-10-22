package nz.mwh.cpsgrace;

public interface Continuation {
    public PendingStep apply(GraceObject value);

    public default PendingStep returning(Context ctx, GraceObject value) {
        return new PendingStep(ctx, this, value);
    }
}
