package nz.mwh.cpsgrace.ast;

import java.util.Collections;
import java.util.List;

public class ConciseEncoding {
    @SuppressWarnings("rawtypes")
    protected static ConsList nil = ConsList.nil();

    protected static <T> ConsList<T> c0N(T value, ConsList<T> list) {
        return ConsList.cons(value, list);
    }

    protected static <T> ConsList<T> o1N(T value) {
        return ConsList.one(value);
    }
    
    protected static <T> ConsList<T> c2N(T value1, T value2) {
        return ConsList.cons(value1, ConsList.one(value2));
    }

    protected static StrLit s0L(String value) {
        return new StrLit(value);
    }

    protected static NumLit n0M(Number value) {
        return new NumLit(value);
    }

    protected static LexReq l0R(String name, ConsList<ASTNode> arguments) {
        if (name.endsWith("(0)")) {
            name = name.substring(0, name.length() - 3);
        }
        return new LexReq(name, arguments.asList(), Collections.emptyList());
    }

    protected static LexReq l0R(String name, ConsList<ASTNode> arguments, ConsList<ASTNode> genericArguments) {
        if (name.endsWith("(0)")) {
            name = name.substring(0, name.length() - 3);
        }
        return new LexReq(name, arguments.asList(), genericArguments.asList());
    }

    protected static DotReq d0R(ASTNode receiver, String name, ConsList<ASTNode> arguments, ConsList<ASTNode> genericArguments) {
        if (name.endsWith("(0)")) {
            name = name.substring(0, name.length() - 3);
        }
        return new DotReq(receiver, name, arguments.asList(), genericArguments.asList());
    }

    protected static Part p0T(String name, ConsList<ASTNode> arguments, ConsList<ASTNode> genericArguments) {
        return new Part(name, arguments.asList(), genericArguments.asList());
    }

    protected static ObjCons o0C(ConsList<ASTNode> body, ConsList<ASTNode> annotations) {
        return new ObjCons(body.asList());
    }

    protected static Block b1K(ConsList<ASTNode> parameters, ConsList<ASTNode> body) {
        return new Block(parameters.asList(), body.asList());
    }

    protected static VarDec v4R(String name, ConsList<ASTNode> declaredType, ConsList<ASTNode> annotations, ConsList<ASTNode> valueExpr) {
        return new VarDec(name, declaredType.single(), annotations.asList(), valueExpr.single());
    }

    protected static DefDec d3F(String name, ConsList<ASTNode> declaredType, ConsList<String> annotations, ASTNode valueExpr) {
        List<ASTNode> annots = annotations.asList().stream().map(x -> (ASTNode)new LexReq(List.of(new Part(x, List.of(), List.of())), "<unknown>")).toList();
        return new DefDec(name, declaredType.single(), annots, valueExpr);
    }

    protected static Assn a5N(ASTNode left, ASTNode right) {
        return new Assn(left, right);
    }

    protected static IdentifierDeclaration i0D(String name, ConsList<ASTNode> declaredType) {
        return new IdentifierDeclaration(name, declaredType.single());
    }

    protected static InterpStr i0S(String pre, ASTNode expr, ASTNode post) {
        return new InterpStr(pre, expr, post);
    }

    protected static MethodDecl m0D(ConsList<Part> parts, ConsList<ASTNode> returnType, ConsList<ASTNode> annotations, ConsList<ASTNode> body) {
        return new MethodDecl(parts.asList(), returnType.single(), annotations.asList(), body.asList());
    }

    protected static ReturnStmt r3T(ASTNode expr) {
        return new ReturnStmt(expr);
    }

    protected static ASTNode c0M(String text) {
        return new Comment(text);
    }

    protected static ASTNode i0M(String name, IdentifierDeclaration asName) {
        return new ImportStmt(name, asName);
    }

    protected static TypeDec t0D(String name, ConsList<ASTNode> genericParams, ASTNode typeExpr) {
        return new TypeDec(name, genericParams.asList(), typeExpr);
    }

    protected static InterfaceCons i0C(ConsList<ASTNode> body) {
        return new InterfaceCons(body.asList().stream().map(x -> (MethSig) x).toList());
    }

    protected static MethSig m0S(ConsList<Part> parts, ConsList<ASTNode> returnType) {
        return new MethSig(parts.asList(), returnType.single());
    }

    protected static Lineup l0N(ConsList<ASTNode> elements) {
        return new Lineup(elements.asList());
    }
    
    protected static String s4F(String pre, String mid, String post) {
        return pre + mid + post;
    }


    protected static final String c9D = "$";
    protected static final String c9B = "\\";
    protected static final String c9Q = "\"";
    protected static final String c9N = "\n";
    protected static final String c9R = "\r";
    protected static final String c9L = "{";
    protected static final String c9S = "*";
    protected static final String c9T = "~";
    protected static final String c9G = "`";
    protected static final String c9C = "^";
    protected static final String c9A = "@";
    protected static final String c9P = "%";
    protected static final String c9M = "&";
    protected static final String c9H = "#";
    protected static final String c9E = "!";

}
 