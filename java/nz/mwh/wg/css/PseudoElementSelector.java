package nz.mwh.wg.css;

public class PseudoElementSelector extends Selector {
    private String pseudoElement;
    
    public PseudoElementSelector(String pseudoElement) {
        this.pseudoElement = pseudoElement;
    }
    
    public String getElement() {
        return pseudoElement;
    }

    public String toString() {
        return "::" + pseudoElement;
    }

}
