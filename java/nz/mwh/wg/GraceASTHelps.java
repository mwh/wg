package nz.mwh.wg;

import nz.mwh.wg.runtime.*;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.grace.*;

public class GraceASTHelps extends nz.mwh.wg.ast.grace.ASTConstructors {
        
    @SuppressWarnings("unchecked")
    public static BaseObject astConstructorPrelude() {
        BaseObject lexicalParent = Evaluator.basePrelude();
        lexicalParent.addMethod("cons(2)", request -> {
            return cons(request.getParts().get(0).getArgs().get(0), (Cons<GraceObject>) request.getParts().get(0).getArgs().get(1));
        });
        lexicalParent.addMethod("nil(0)", request -> {
            return new Cons<GraceObject>();
        });
        lexicalParent.addMethod("objectConstructor(1)", request -> {
            Cons<ASTNode> body = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(0);
            return objectConstructor(body);
        });
        lexicalParent.addMethod("numberNode(1)", request -> {
            return numberNode((int) ((GraceNumber) request.getParts().get(0).getArgs().get(0)).getValue());
        });
        lexicalParent.addMethod("stringNode(1)", request -> {
            return stringNode(((GraceString) request.getParts().get(0).getArgs().get(0)).getValue());
        });
        lexicalParent.addMethod("requestPart(2)", request -> {
            String name = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            Cons<ASTNode> args = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(1);
            return requestPart(name, args);
        });
        lexicalParent.addMethod("lexicalRequest(1)", request -> {
            Cons<RequestPart> parts = (Cons<RequestPart>) request.getParts().get(0).getArgs().get(0);
            return lexicalRequest(parts);
        });
        lexicalParent.addMethod("block(2)", request -> {
            GraceObject params = request.getParts().get(0).getArgs().get(0);
            GraceObject body = request.getParts().get(0).getArgs().get(1);
            return block((Cons<ASTNode>) params, (Cons<ASTNode>) body);
        });
        lexicalParent.addMethod("defDecl(4)", request -> {
            String name = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            Cons<ASTNode> typeCons = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(1);
            Cons<String> annotations = (Cons<String>) request.getParts().get(0).getArgs().get(2);
            ASTNode value = (ASTNode) request.getParts().get(0).getArgs().get(3);
            return defDecl(name, typeCons, annotations, value);
        });
        lexicalParent.addMethod("varDecl(4)", request -> {
            String name = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            Cons<ASTNode> typeCons = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(1);
            Cons<String> annotations = (Cons<String>) request.getParts().get(0).getArgs().get(2);
            Cons<ASTNode> value = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(3);
            return varDecl(name, typeCons, annotations, value);
        });
        lexicalParent.addMethod("methodDecl(4)", request -> {
            Cons<DeclarationPart> parts = (Cons<DeclarationPart>) request.getParts().get(0).getArgs().get(0);
            Cons<ASTNode> typeCons = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(1);
            Cons<String> anns = (Cons<String>) request.getParts().get(0).getArgs().get(2);
            Cons<ASTNode> body = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(3);
            return methodDecl(parts, typeCons, anns, body);
        });
        lexicalParent.addMethod("declarationPart(2)", request -> {
            String name = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            Cons<IdentifierDeclaration> parameters = (Cons<IdentifierDeclaration>) request.getParts().get(0).getArgs().get(1);
            return declarationPart(name, parameters);
        });
        lexicalParent.addMethod("explicitRequest(3)", request -> {
            String location = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            ASTNode receiver = (ASTNode) request.getParts().get(0).getArgs().get(1);
            Cons<RequestPart> parts = (Cons<RequestPart>) request.getParts().get(0).getArgs().get(2);
            return explicitRequest(location, receiver, parts);
        });
        lexicalParent.addMethod("assign(2)", request -> {
            ASTNode target = (ASTNode) request.getParts().get(0).getArgs().get(0);
            ASTNode value = (ASTNode) request.getParts().get(0).getArgs().get(1);
            return assign(target, value);
        });
        lexicalParent.addMethod("returnStmt(1)", request -> {
            ASTNode value = (ASTNode) request.getParts().get(0).getArgs().get(0);
            return returnStmt(value);
        });
        lexicalParent.addMethod("identifierDeclaration(2)", request -> {
            String name = ((GraceString) request.getParts().get(0).getArgs().get(0)).getValue();
            Cons<ASTNode> typeCons = (Cons<ASTNode>) request.getParts().get(0).getArgs().get(1);
            return identifierDeclaration(name, typeCons);
        });
        lexicalParent.addMethod("comment(1)", request -> {
            return comment(((GraceString) request.getParts().get(0).getArgs().get(0)).getValue());
        });

        return lexicalParent;

    }

    static Object test = null;

}

