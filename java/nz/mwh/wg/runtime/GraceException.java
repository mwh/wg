package nz.mwh.wg.runtime;

import java.lang.Iterable;

import nz.mwh.wg.Visitor;

public class GraceException extends RuntimeException implements GraceObject {
    private String message;
    private Iterable<String> callStack;

    public GraceException(Visitor<GraceObject> evaluator, String message) {
        this.callStack = evaluator.getStack();
        this.message = message;
    }

    public String getMessage() {
        return message;
    }

    public Iterable<String> getCallStack() {
        return callStack;
    }
    
    @Override
    public String toString() {
        return "GraceException: " + message;
    }

    @Override
    public GraceObject request(Request request) {
        throw new UnsupportedOperationException("Unimplemented method 'request'");
    }

    @Override
    public GraceObject findReceiver(String name) {
        throw new RuntimeException("No such method in scope: " + name);
    }

}
