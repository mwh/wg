package nz.mwh.wg.runtime;

import java.lang.Iterable;

import nz.mwh.wg.Visitor;

public class GraceException extends RuntimeException implements GraceObject {
    private String message;
    private GraceExceptionKind kind;
    private Iterable<String> callStack;

    public GraceException(Visitor<GraceObject> evaluator, String message) {
        this.callStack = evaluator.getStack();
        this.message = message;
        this.kind = GraceExceptionKind.BASE;
    }

    public GraceException(Visitor<GraceObject> evaluator, GraceExceptionKind kind, String message) {
        this.callStack = evaluator.getStack();
        this.message = message;
        this.kind = kind;
    }

    public String getMessage() {
        return message;
    }

    public Iterable<String> getCallStack() {
        return callStack;
    }

    public GraceExceptionKind getKind() {
        return kind;
    }
    
    @Override
    public String toString() {
        return kind.getName() + ": " + message;
    }

    @Override
    public GraceObject request(Request request) {
        switch (request.getName()) {
            case "asString(0)":
                return new GraceString(toString());
            case "reraise(0)":
                throw this;
        }
        throw new GraceException(request.getVisitor(), "No such method in ExceptionPacket: " + request.getName());
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }

}
