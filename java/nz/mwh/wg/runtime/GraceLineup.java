package nz.mwh.wg.runtime;

import java.util.List;
import java.util.ArrayList;

public class GraceLineup implements GraceObject {
    private List<GraceObject> elements;

    public GraceLineup(List<GraceObject> elements) {
        this.elements = elements;
    }

    public List<GraceObject> getElements() {
        return elements;
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("[");
        for (int i = 0; i < elements.size(); i++) {
            if (i > 0) {
                sb.append(", ");
            }
            sb.append(elements.get(i).toString());
        }
        sb.append("]");
        return sb.toString();
    }

    @Override
    public GraceObject request(Request request) {
        List<RequestPartR> parts = request.getParts();
        if (parts.size() == 1) {
            String name = parts.get(0).getName();
            if (name.equals("do") || name.equals("each")) {
                GraceObject block = parts.get(0).getArgs().get(0);
                for (GraceObject element : elements) {
                    block.request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of(element)))));
                }
                return GraceDone.done;
            } else if (name.equals("map")) {
                GraceObject block = parts.get(0).getArgs().get(0);
                List<GraceObject> results = new ArrayList<>();
                for (GraceObject element : elements) {
                    results.add(block.request(new Request(request.getVisitor(), List.of(new RequestPartR("apply", List.of(element))))));
                }
                return new GraceLineup(results);
            } else if (name.equals("++")) {
                GraceObject other = parts.get(0).getArgs().get(0);
                if (other instanceof GraceLineup otherLineup) {
                    List<GraceObject> combined = new ArrayList<>(elements);
                    combined.addAll(otherLineup.elements);
                    return new GraceLineup(combined);
                }
                throw new RuntimeException("Cannot concatenate lineup with " + other);
            } else if (name.equals("asString")) {
                StringBuilder sb = new StringBuilder();
                sb.append("[");
                for (int i = 0; i < elements.size(); i++) {
                    if (i > 0) {
                        sb.append(", ");
                    }
                    sb.append(elements.get(i).request(new Request(request.getVisitor(), List.of(new RequestPartR("asString", List.of())))).toString());
                }
                sb.append("]");
                return new GraceString(sb.toString());
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in Lineup: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}
