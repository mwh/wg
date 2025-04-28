package nz.mwh.wg.runtime;

public class GraceUninitialised implements GraceObject {
    public static GraceUninitialised uninitialised = new GraceUninitialised();
    
    @Override
    public GraceObject request(Request request) {
        throw new GraceException(request.getVisitor(), "Cannot request method on uninitialised value: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        return null;
    }
}