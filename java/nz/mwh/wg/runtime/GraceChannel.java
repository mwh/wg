package nz.mwh.wg.runtime;

import nz.mwh.wg.Dala;

public class GraceChannel implements GraceObject {
    private final GracePort sendPort;
    private final GracePort receivePort;

    public GraceChannel(GracePort port1, GracePort port2) {
        this.sendPort = port1;
        this.receivePort = port2;
    }

    public GracePort getSendPort() {
        return sendPort;
    }

    public GracePort getReceivePort() {
        return receivePort;
    }

    public void send(GraceObject message) throws InterruptedException {
        sendPort.send(message);
    }

    public GraceObject receive() throws InterruptedException {
        GraceObject received = receivePort.receive();
        return received;
    }

    @Override
    public GraceObject request(Request request) {
        if ("send(1)".equals(request.getName())) {
            try {
                GraceObject obj = request.getParts().get(0).getArgs().get(0);
                switch (obj) {
                    case BaseObject b:
                        b.incRefCount();
                        if (b.isIso() && (Dala.getIsoWhen() == Dala.IsoWhen.THREAD || Dala.getIsoWhen() == Dala.IsoWhen.DEREFERENCE_THREAD) && b.getRefCount() > 1) {
                            throw new GraceException(request.getVisitor(), GraceExceptionKind.BASE, "aliased iso object sent across thread boundary");
                        }
                        b.decRefCount();
                        obj = b.beReturned();
                        break;
                    default:
                }
                send(obj);
                return GraceDone.done;
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        } else if ("receive(0)".equals(request.getName())) {
            try {
                return receive();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
        throw new GraceException(request.getVisitor(), GraceExceptionKind.BASE, "Unimplemented method " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }
}