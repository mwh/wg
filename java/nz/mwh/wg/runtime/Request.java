package nz.mwh.wg.runtime;

import java.util.List;
import java.util.stream.Collectors;

import nz.mwh.wg.Visitor;

public class Request {
    List<RequestPartR> parts;
    private Visitor<GraceObject> visitor;
    private String location = "<unknown>";

    public Request(Visitor<GraceObject> visitor, List<RequestPartR> parts) {
        this.visitor = visitor;
        this.parts = parts;
    }

    public Request(Visitor<GraceObject> visitor, List<RequestPartR> parts, String location) {
        this.visitor = visitor;
        this.parts = parts;
        this.location = location;
    }

    public List<RequestPartR> getParts() {
        return parts;
    }

    public String getName() {
        return parts.stream().map(x -> x.getName() + "(" + x.getArgs().size() + ")").collect(Collectors.joining(""));
    }

    public String getLocation() {
        return location;
    }

    public Visitor<GraceObject> getVisitor() {
        return visitor;
    }

    public static Request nullary(Visitor<GraceObject> visitor, String name) {
        return new Request(visitor, List.of(new RequestPartR(name, List.of())));
    }

    public static Request unary(Visitor<GraceObject> visitor, String name, GraceObject arg) {
        return new Request(visitor, List.of(new RequestPartR(name, List.of(arg))));
    }

    public static Request binary(Visitor<GraceObject> visitor, String name, GraceObject arg1, GraceObject arg2) {
        return new Request(visitor, List.of(new RequestPartR(name, List.of(arg1, arg2))));
    }

    public static Request twoPart(Visitor<GraceObject> visitor, String name1, String name2, GraceObject arg1, GraceObject arg2) {
        return new Request(visitor, List.of(new RequestPartR(name1, List.of(arg1)), new RequestPartR(name2, List.of(arg2))));
    }
}
