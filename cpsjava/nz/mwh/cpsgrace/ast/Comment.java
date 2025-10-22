package nz.mwh.cpsgrace.ast;

import nz.mwh.cpsgrace.CPS;

public class Comment extends ASTNode {

    private String text;

    public Comment(String text) {
        this.text = text;
    }

    public String getText() {
        return text;
    }

    public String toString() {
        return "comment(\"" +
            text.replace("\\", "\\\\")
                .replace("\n", "\\n")
                .replace("\r", "\\r")
                .replace("\"", "\\\"")
            + "\")";
    }

    @Override
    public CPS toCPS() {
        return (_, cont) -> cont.apply(null);
    }

}
