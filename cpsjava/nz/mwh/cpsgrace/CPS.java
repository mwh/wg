package nz.mwh.cpsgrace;

public interface CPS {
    PendingStep run(Context ctx, Continuation continuation);
}
