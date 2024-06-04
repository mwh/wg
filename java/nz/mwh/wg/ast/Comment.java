package nz.mwh.wg.ast;

import nz.mwh.wg.Visitor;

public class Comment extends ASTNode {

    private String text;

    public Comment(String text) {
        this.text = text;
    }

    public String getText() {
        return text;
    }

    public String toString() {
        return "comment(\"" + text + "\")";
    }

    @Override
    public <T> T accept(T context, Visitor<T> visitor) {
        return visitor.visit(context, this);
    }

}
