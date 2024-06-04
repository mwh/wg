package nz.mwh.wg;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Assign;
import nz.mwh.wg.ast.Block;
import nz.mwh.wg.ast.Cons;
import nz.mwh.wg.ast.Comment;
import nz.mwh.wg.ast.DeclarationPart;
import nz.mwh.wg.ast.DefDecl;
import nz.mwh.wg.ast.ExplicitRequest;
import nz.mwh.wg.ast.IdentifierDeclaration;
import nz.mwh.wg.ast.LexicalRequest;
import nz.mwh.wg.ast.MethodDecl;
import nz.mwh.wg.ast.NumberNode;
import nz.mwh.wg.ast.ObjectConstructor;
import nz.mwh.wg.ast.RequestPart;
import nz.mwh.wg.ast.ReturnStmt;
import nz.mwh.wg.ast.StringNode;
import nz.mwh.wg.ast.VarDecl;

public class ASTConstructors {
        static <T> Cons<T> cons(T head, Cons<T> tail) {
        return new Cons<>(head, tail);
    }

    @SuppressWarnings("rawtypes")
    static Cons nil = new Cons();

    static ObjectConstructor objectConstructor(Cons<ASTNode> body) {
        return new ObjectConstructor(body);
    }

    static LexicalRequest lexicalRequest(Cons<RequestPart> parts) {
        return new LexicalRequest(parts);
    }

    static RequestPart requestPart(String name, Cons<ASTNode> args) {
        return new RequestPart(name, args);
    }

    static NumberNode numberNode(double value) {
        return new NumberNode(value);
    }

    static StringNode stringNode(String value) {
        return new StringNode(value);
    }

    static IdentifierDeclaration identifierDeclaration(String name) {
        return new IdentifierDeclaration(name);
    }

    static DefDecl defDecl(String name, Cons<String> annotations, ASTNode value) {
        return new DefDecl(name, annotations, value);
    }

    static VarDecl varDecl(String name, Cons<String> annotations, ASTNode value) {
        return new VarDecl(name, annotations, value);
    }

    static MethodDecl methodDecl(Cons<DeclarationPart> parts, Cons<ASTNode> body) {
        return new MethodDecl(parts, body);
    }

    static DeclarationPart declarationPart(String name, Cons<IdentifierDeclaration> parameters) {
        return new DeclarationPart(name, parameters);
    }

    static ExplicitRequest explicitRequest(ASTNode receiver, Cons<RequestPart> parts) {
        return new ExplicitRequest("<unknown>", receiver, parts);
    }

    static ExplicitRequest explicitRequest(String location, ASTNode receiver, Cons<RequestPart> parts) {
        return new ExplicitRequest(location, receiver, parts);
    }

    static Assign assign(ASTNode target, ASTNode value) {
        return new Assign(target, value);
    }

    static Block block(Cons<ASTNode> parameters, Cons<ASTNode> body) {
        return new Block(parameters, body);
    }

    static ReturnStmt returnStmt(ASTNode value) {
        return new ReturnStmt(value);
    }

    protected static Comment comment(String value) {
        return new Comment(value);
    }

}
