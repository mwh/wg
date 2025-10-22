package nz.mwh.cpsgrace.ast;

public class ASTEncoding {
    @SuppressWarnings("rawtypes")
    protected static ConsList nil = ConsList.nil();

    protected static <T> ConsList<T> cons(T value, ConsList<T> list) {
        return ConsList.cons(value, list);
    }

    protected static <T> ConsList<T> one(T value) {
        return ConsList.one(value);
    }

    protected static StrLit strLit(String value) {
        return new StrLit(value);
    }

    protected static NumLit numLit(Number value) {
        return new NumLit(value);
    }

    protected static LexReq lexReq(ConsList<Part> parts) {
        return new LexReq(parts.asList());
    }

    protected static DotReq dotReq(ASTNode receiver, ConsList<Part> parts) {
        return new DotReq(receiver, parts.asList());
    }

    protected static Part part(String name, ConsList<ASTNode> arguments) {
        return new Part(name, arguments.asList());
    }

    protected static ObjCons objCons(ConsList<ASTNode> body, ConsList<ASTNode> annotations) {
        return new ObjCons(body.asList());
    }

    protected static Block block(ConsList<ASTNode> parameters, ConsList<ASTNode> body) {
        return new Block(parameters.asList(), body.asList());
    }

    protected static VarDec varDec(String name, ConsList<ASTNode> declaredType, ConsList<ASTNode> annotations, ConsList<ASTNode> valueExpr) {
        return new VarDec(name, declaredType.single(), annotations.asList(), valueExpr.single());
    }

    protected static DefDec defDec(String name, ConsList<ASTNode> declaredType, ConsList<String> annotations, ASTNode valueExpr) {
        return new DefDec(name, declaredType.single(), annotations.asList(), valueExpr);
    }

    protected static Assn assn(ASTNode left, ASTNode right) {
        return new Assn(left, right);
    }

    protected static IdentifierDeclaration identifierDeclaration(String name, ConsList<ASTNode> declaredType) {
        return new IdentifierDeclaration(name, declaredType.single());
    }

    protected static InterpStr interpStr(String pre, ASTNode expr, ASTNode post) {
        return new InterpStr(pre, expr, post);
    }

    protected static MethodDecl methDec(ConsList<Part> parts, ConsList<ASTNode> returnType, ConsList<ASTNode> annotations, ConsList<ASTNode> body) {
        return new MethodDecl(parts.asList(), returnType.single(), annotations.asList(), body.asList());
    }

    protected static ReturnStmt returnStmt(ASTNode expr) {
        return new ReturnStmt(expr);
    }

    protected static ASTNode comment(String text) {
        return new Comment(text);
    }

    protected static ASTNode importStmt(String name, IdentifierDeclaration asName) {
        return new ImportStmt(name, asName);
    }
    
    protected static String safeStr(String pre, String mid, String post) {
        return pre + mid + post;
    }


    protected static final String charDollar = "$";
    protected static final String charBackslash = "\\";
    protected static final String charDQuote = "\"";
    protected static final String charLF = "\n";
    protected static final String charCR = "\r";
    protected static final String charLBrace = "{";
    protected static final String charStar = "*";
    protected static final String charTilde = "~";
    protected static final String charBacktick = "`";
    protected static final String charCaret = "^";
    protected static final String charAt = "@";
    protected static final String charPercent = "%";
    protected static final String charAmp = "&";
    protected static final String charHash = "#";
    protected static final String charExclam = "!";
}
