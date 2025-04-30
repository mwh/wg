package nz.mwh.wg.runtime;

import nz.mwh.wg.Dala;

public class IsoProxy implements GraceObject {
    private final GraceObject target;
    private GraceObject predecessor;
    private boolean alive = true;

    public IsoProxy(GraceObject target, IsoProxy predecessor) {
        this.target = target;
        this.predecessor = predecessor;
    }

    @Override
    public GraceObject request(Request request) {
        if (!alive) {
            throw new GraceException(request.getVisitor(), GraceExceptionKind.BASE, "method requested where moved iso no longer present");
        }
        return target.request(request);
    }

    @Override
    public GraceObject findReceiver(String name) {
        return target.findReceiver(name);
    }

    @Override
    public GraceObject beAssigned(GraceObject container, String name) {
        if (Dala.getIsoMove() == Dala.IsoMove.MOVE) {
            IsoProxy next = new IsoProxy(target.beReturned(), this);
            alive = false;
            return next;
        }
        return target.beAssigned(container, name);
    }

    public GraceObject beReturned() {
        target.beReturned();
        return this;
    };

    public boolean isBeingReturned() {
        return target.isBeingReturned();
    };

    public void incRefCount() {
        if (alive)
            target.incRefCount();
    }

    public void decRefCount() {
        if (alive)
            target.decRefCount();
    }

    public int getRefCount() {
        return ((BaseObject)target).getRefCount();
    }

    public void discard() {
        if (alive)
            target.discard();
    }

    
}
