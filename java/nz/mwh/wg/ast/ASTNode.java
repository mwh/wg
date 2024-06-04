package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public abstract class ASTNode{

    public abstract <T> T accept(T context, Visitor<T> visitor);

}