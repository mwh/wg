package nz.mwh.wg.runtime;

public interface GraceObject {
    GraceObject request(Request request);
    GraceObject findReceiver(String name);

    default GraceObject beReturned() {
        return this;
    };
    default boolean isBeingReturned() {
        return false;
    };
    default void discard() {};
    default void incRefCount() {};
    default void decRefCount() {};
    default GraceObject beAssigned(GraceObject container, String name) {
        return this;
    };
    default int getRefCount() {
        return -1;
    };
    default boolean isIso() {
        return false;
    };
    default boolean isUnsafe() {
        return false;
    };
    default boolean isLocal() {
        return false;
    };
    default boolean isImm() {
        return false;
    };
}
