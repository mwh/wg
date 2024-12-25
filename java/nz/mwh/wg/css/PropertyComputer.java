package nz.mwh.wg.css;

import java.util.List;
import java.util.Map;

import nz.mwh.wg.ast.ASTNode;

public class PropertyComputer {
    private List<Property> properties;
    private ASTNode node;
    private Map<String, Object> attributes;
    private Object value;
    private String defaultFormat;

    private String content;
    private String label;
    
    public PropertyComputer(List<Property> properties, ASTNode node, Map<String, Object> attributes, Object value, String defaultFormat) {
        this.properties = properties;
        this.node = node;
        this.attributes = attributes;
        this.value = value;
        this.defaultFormat = defaultFormat;

        compute();        
    }

    private void compute() {
        boolean setContent = false;
        for (Property p : properties) {
            switch (p.getName()) {
                case "content":
                content = p.getValue(attributes);
                setContent = true;
                break;
                case "label":
                label = p.getValue(attributes);
                break;
            }
        }
        if (!setContent) {
            content = String.format(defaultFormat, node, value, label);
        }
    }

    public String getContent() {
        return content;
    }

    public String getLabel() {
        return label;
    }

}
