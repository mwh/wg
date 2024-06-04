package nz.mwh.wg.ast.grace;

import nz.mwh.wg.ast.ASTNode;

public class ASTConstructors {
    protected static <T> Cons<T> cons(T head, Cons<T> tail) {
        return new Cons<>(head, tail);
    }

    @SuppressWarnings("rawtypes")
    protected static Cons nil = new Cons();

    protected static ObjectConstructor objectConstructor(Cons<ASTNode> body) {
        return new ObjectConstructor(body);
    }

    protected static LexicalRequest lexicalRequest(Cons<RequestPart> parts) {
        return new LexicalRequest(parts);
    }

    protected static RequestPart requestPart(String name, Cons<ASTNode> args) {
        return new RequestPart(name, args);
    }

    protected static NumberNode numberNode(double value) {
        return new NumberNode(value);
    }

    protected static StringNode stringNode(String value) {
        return new StringNode(value);
    }

    protected static IdentifierDeclaration identifierDeclaration(String name) {
        return new IdentifierDeclaration(name);
    }

    protected static DefDecl defDecl(String name, Cons<String> annotations, ASTNode value) {
        return new DefDecl(name, annotations, value);
    }

    protected static VarDecl varDecl(String name, Cons<String> annotations, ASTNode value) {
        return new VarDecl(name, annotations, value);
    }

    protected static MethodDecl methodDecl(Cons<DeclarationPart> parts, Cons<ASTNode> body) {
        return new MethodDecl(parts, body);
    }

    protected static DeclarationPart declarationPart(String name, Cons<IdentifierDeclaration> parameters) {
        return new DeclarationPart(name, parameters);
    }

    protected static ExplicitRequest explicitRequest(ASTNode receiver, Cons<RequestPart> parts) {
        return new ExplicitRequest("<unknown>", receiver, parts);
    }

    protected static ExplicitRequest explicitRequest(String location, ASTNode receiver, Cons<RequestPart> parts) {
        return new ExplicitRequest(location, receiver, parts);
    }

    protected static Assign assign(ASTNode target, ASTNode value) {
        return new Assign(target, value);
    }

    protected static Block block(Cons<ASTNode> parameters, Cons<ASTNode> body) {
        return new Block(parameters, body);
    }

    protected static ReturnStmt returnStmt(ASTNode value) {
        return new ReturnStmt(value);
    }

}
