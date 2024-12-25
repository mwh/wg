package nz.mwh.wg.css;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.runtime.GraceObject;

public class StackFrameSelector extends Selector {
    private String methodName;
    
    public StackFrameSelector(String methodName) {
        this.methodName = methodName;
    }
    
    public String getMethodName() {
        return methodName;
    }
    
    public String toString() {
        return "." + methodName;
    }
    
    @Override
    public boolean matchFrameAt(String name, ASTNode node, GraceObject scope) {
        return name.startsWith(methodName + "(");
    }
}
