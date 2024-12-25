package nz.mwh.wg.css;

import nz.mwh.wg.ast.ASTNode;

public class Trace {
    private ASTNode node;
    private PropertyComputer computedProperties;
    private Rule rule;

    public Trace(ASTNode node, PropertyComputer computedProperties, Rule rule) {
        this.node = node;
        this.computedProperties = computedProperties;
        this.rule = rule;
    }

    public ASTNode getNode() {
        return node;
    }

    public String getLabel() {
        return computedProperties.getLabel();
    }

    public String toString() {
        var label = getLabel();
        if (label == null) {
            return rule.selectorString() + " => " + computedProperties.getContent();
        }
        return label + " => " + computedProperties.getContent();
    }

    public Rule getRule() {
        return rule;
    }

}
