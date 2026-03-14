package nz.mwh.wg.runtime;

import java.util.List;
import java.util.ArrayList;
import java.util.Arrays;

public class GracePrimitiveArray implements GraceObject {
    private GraceObject[] elements;

    public GracePrimitiveArray(int size) {
        this.elements = new GraceObject[size];
        Arrays.fill(this.elements, GraceUninitialised.uninitialised);
    }

    public List<GraceObject> getElements() {
        return Arrays.asList(elements);
    }

    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("primitiveArray[");
        for (int i = 0; i < elements.length; i++) {
            if (i > 0) {
                sb.append(", ");
            }
            sb.append(elements[i].toString());
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
            } else if (name.equals("at")) {
                int index = GraceNumber.assertInt(parts.get(0).getArgs().get(0), "index");
                if (index < 0 || index >= elements.length) {
                    throw new GraceException(request.getVisitor(), "Primitive Array index out of bounds: " + index);
                }
                GraceObject result = elements[index];
                if (result instanceof GraceUninitialised) {
                    throw new GraceException(request.getVisitor(), "Cannot access uninitialised array element at index " + index);
                }
                return result;
            } else if (name.equals("size")) {
                return new GraceNumber(elements.length);
            } else if (name.equals("asString")) {
                return new GraceString("primitiveArray.new(" + elements.length + ")");
            } else if (name.equals("asDebugString")) {
                StringBuilder sb = new StringBuilder();
                sb.append("primitiveArray.of[");
                for (int i = 0; i < elements.length; i++) {
                    if (i > 0) {
                        sb.append(", ");
                    }
                    sb.append(elements[i].request(new Request(request.getVisitor(), List.of(new RequestPartR("asDebugString", List.of())))).toString());
                }
                sb.append("]");
                return new GraceString(sb.toString());
            }
        } else if (parts.size() == 2) {
            String name1 = parts.get(0).getName();
            String name2 = parts.get(1).getName();
            if (name1.equals("at") && name2.equals("put")) {
                int index = GraceNumber.assertInt(parts.get(0).getArgs().get(0), "index");
                if (index < 0 || index >= elements.length) {
                    throw new GraceException(request.getVisitor(), "Primitive Array index out of bounds: " + index);
                }
                GraceObject value = parts.get(1).getArgs().get(0);
                elements[index] = value;
                return GraceDone.done;
            }
        }
        throw new GraceException(request.getVisitor(), "No such method in primitive array: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }
}
