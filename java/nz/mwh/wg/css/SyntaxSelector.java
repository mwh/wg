package nz.mwh.wg.css;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Assign;
import nz.mwh.wg.ast.LexicalRequest;
import nz.mwh.wg.ast.MethodDecl;
import nz.mwh.wg.ast.ObjectConstructor;
import nz.mwh.wg.ast.VarDecl;
import nz.mwh.wg.runtime.GraceObject;

public class SyntaxSelector extends Selector {
    private String syntax;
    
    public SyntaxSelector(String syntax) {
        this.syntax = syntax;
    }
    
    public String getSyntax() {
        return syntax;
    }
    
    public boolean matches(StackFrameSelector stackFrame) {
        return false;
    }
    
    public String toString() {
        String result = syntax;
        for (PseudoclassSelector p : getPseudoclasses()) {
            result += p.toString();
        }
        if (getPseudoElement() != null) {
            result += getPseudoElement().toString();
        }
        return result;
    }

    // @Override
    public boolean matchAt(VarDecl node, GraceObject scope) {
        if ("var".equals(syntax)) {
            for (PseudoclassSelector pseudo : getPseudoclasses()) {
                if ("name".equals(pseudo.getPseudoclass())) {
                    return pseudo.getArgument().equals(node.getName());
                } else {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    @Override
    public boolean matchAt(MethodDecl node, GraceObject scope) {
        return "method".equals(syntax);
    }

    @Override
    public boolean matchAt(ObjectConstructor node, GraceObject scope) {
        return "object".equals(syntax);
    }
    
    @Override
    public boolean matchAt(Assign node, GraceObject scope) {
        if ("assign".equals(syntax)) {
            for (PseudoclassSelector pseudo : getPseudoclasses()) {
                if ("name".equals(pseudo.getPseudoclass())) {
                    if (node.getTarget() instanceof LexicalRequest target) {
                        String name = target.getParts().get(0).getName();
                        return pseudo.getArgument().equals(name);
                    } else {
                        return false;
                    }
                } else {
                    return false;
                }
            }
            return true;
        }
        return false;
    }
}
