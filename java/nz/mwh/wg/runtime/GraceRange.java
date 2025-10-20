package nz.mwh.wg.runtime;

public class GraceRange implements GraceObject {
    double start;
    double end;
    double step;

    public GraceRange(double start, double end) {
        this.start = start;
        this.end = end;
        this.step = 1;
    }

    public String toString() {
        String st = (start == (int) start) ? "" + (int)start : "" + start;
        String en = (end == (int) end) ? "" + (int)end : "" + end;
        return st + ".." + en;
    }

    @Override
    public GraceObject request(Request request) {
        switch(request.getName()) {
            case "asString(0)": return new GraceString(toString());
            case "each(1)": {
                GraceObject block = request.getParts().getFirst().getArgs().getFirst();
                for (double val = start; val <= end; val += step) {
                    Request req = Request.unary(request.getVisitor(), "apply", new GraceNumber(val));
                    block.request(req);
                }
                return GraceDone.done;
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in Range: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}
