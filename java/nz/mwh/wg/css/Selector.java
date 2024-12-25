package nz.mwh.wg.css;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Assign;
import nz.mwh.wg.ast.MethodDecl;
import nz.mwh.wg.ast.ObjectConstructor;
import nz.mwh.wg.ast.VarDecl;
import nz.mwh.wg.runtime.GraceObject;

public abstract class Selector {
    // public abstract Selector getNext();
    private List<Selector> successors;
    private List<PseudoclassSelector> pseudoclasses;
    private PseudoElementSelector pseudoElement;

    public Selector() {
        this.successors = new ArrayList<>();
        this.pseudoclasses = new ArrayList<>();
    }

    public List<Selector> successors() {
        return successors;
    }

    public void addSuccessor(Selector successor) {
        successors.add(successor);
    }

    public List<Selector> successors(ASTNode node, GraceObject scope) {
        List<Selector> result = new ArrayList<>();
        if (matchAt(node, scope)) {
            result.addAll(this.successors);
        }
        result.add(this);
        return result;
    }

    public List<Selector> frameSuccessors(String name, ASTNode node, GraceObject scope) {
        List<Selector> result = new ArrayList<>();
        if (matchFrameAt(name, node, scope)) {
            result.addAll(this.successors);
        }
        if (!result.contains(this))
            result.add(this);
        return result;
    }

    public Selector getNext() {
        if (successors.size() > 0) {
            return successors.get(0);
        } else {
            return null;
        }
    }

    public void addPseudoclass(PseudoclassSelector pseudoclass) {
        pseudoclasses.add(pseudoclass);
    }

    public List<PseudoclassSelector> getPseudoclasses() {
        return Collections.unmodifiableList(pseudoclasses);
    }

    public void setPseudoElement(PseudoElementSelector pseudoElement) {
        this.pseudoElement = pseudoElement;
    }

    public PseudoElementSelector getPseudoElement() {
        return pseudoElement;
    }

    public boolean succeedAt(ASTNode node, GraceObject scope) {
        if (successors.size() != 0) {
            return false;
        }
        return matchAt(node, scope);
    }

    public boolean matchFrameAt(String name, ASTNode node, GraceObject scope) {
        return false;
    }

    public boolean match(ASTNode node, GraceObject scope) {
        return matchAt(node, scope);
    }
    
    public boolean matchAt(ASTNode node, GraceObject scope) {
        switch (node) {
            case VarDecl varDecl:
                return matchAt(varDecl, scope);
            case MethodDecl methodDecl:
                return matchAt(methodDecl, scope);
            case ObjectConstructor objectConstructor:
                return matchAt(objectConstructor, scope);
            case Assign assign:
                return matchAt(assign, scope);
            default:
                return false;
        }
    }

    protected boolean matchAt(VarDecl node, GraceObject scope) {
        return false;
    }

    protected boolean matchAt(MethodDecl node, GraceObject scope) {
        return false;
    }
    
    protected boolean matchAt(ObjectConstructor node, GraceObject scope) {
        return false;
    }

    protected boolean matchAt(Assign node, GraceObject scope) {
        return false;
    }
}
