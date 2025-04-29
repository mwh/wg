package nz.mwh.wg.runtime;

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
                send(request.getParts().get(0).getArgs().get(0));
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