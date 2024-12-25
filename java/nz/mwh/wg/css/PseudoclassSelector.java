package nz.mwh.wg.css;

public class PseudoclassSelector extends Selector {
    private String pseudoclass;
    private String argument;
    
    public PseudoclassSelector(String pseudoclass, String argument) {
        this.pseudoclass = pseudoclass;
        this.argument = argument;
    }
    
    public String getPseudoclass() {
        return pseudoclass;
    }

    public String getArgument() {
        return argument;
    }
    
    public boolean matches(StackFrameSelector stackFrame) {
        return false;
    }
    
    public String toString() {
        return ":" + pseudoclass + (argument == null ? "" : "(" + argument + ")");
    }
    
}
