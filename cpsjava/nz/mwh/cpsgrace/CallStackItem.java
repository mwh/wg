package nz.mwh.cpsgrace;

public record CallStackItem(String functionName, CallStackItem caller) {

}
