package nz.mwh.cpsgrace.ast;

import java.util.List;

import nz.mwh.cpsgrace.Context;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.Start;
import nz.mwh.cpsgrace.objects.GraceNumber;
import nz.mwh.cpsgrace.objects.GraceString;

public class Converter {
    
    Context ctx = new Context();
    {
        Start.addPrelude(ctx);
    }

    public ASTNode convertNode(GraceObject node) {
        GraceObject nodeKindObj = requestSync(node, "kind", List.of());
        String nodeKind = GraceString.assertString(nodeKindObj).toString();
        switch (nodeKind) {
            case "objCons" -> {
                GraceObject bodyObj = requestSync(node, "body", List.of());
                List<ASTNode> body = convertNodeList(bodyObj);
                return new ObjCons(body);
            }
            case "numLit" -> {
                GraceObject valueObj = requestSync(node, "value", List.of());
                Number num = GraceNumber.numberValue(valueObj);
                return new NumLit(num);
            }
            case "strLit" -> {
                GraceObject valueObj = requestSync(node, "value", List.of());
                String str = GraceString.assertString(valueObj).toString();
                return new StrLit(str);
            }
            case "lexReq" -> {
                GraceObject partsObj = requestSync(node, "parts", List.of());
                List<ASTNode> parts = convertNodeList(partsObj);
                return new LexReq(parts.stream().map(x -> (Part)x).toList());
            }
            case "part" -> {
                GraceObject nameObj = requestSync(node, "name", List.of());
                String name = GraceString.assertString(nameObj).toString();
                GraceObject paramsObj = requestSync(node, "parameters", List.of());
                List<ASTNode> params = convertNodeList(paramsObj);
                return new Part(name, params);
            }
            case "dotReq" -> {
                GraceObject receiverObj = requestSync(node, "receiver", List.of());
                ASTNode receiver = convertNode(receiverObj);
                GraceObject partsObj = requestSync(node, "parts", List.of());
                List<ASTNode> parts = convertNodeList(partsObj);
                return new DotReq(receiver, parts.stream().map(x -> (Part)x).toList());
            }
            case "assn" -> {
                GraceObject leftObj = requestSync(node, "left", List.of());
                ASTNode left = convertNode(leftObj);
                GraceObject rightObj = requestSync(node, "right", List.of());
                ASTNode right = convertNode(rightObj);
                return new Assn(left, right);
            }
            case "identifierDeclaration" -> {
                GraceObject nameObj = requestSync(node, "name", List.of());
                String name = GraceString.assertString(nameObj).toString();
                GraceObject dtypeObj = requestSync(node, "decType", List.of());
                ASTNode dtype = convertNode(dtypeObj);
                return new IdentifierDeclaration(name, dtype);
            }
            case "returnStmt" -> {
                GraceObject valueObj = requestSync(node, "value", List.of());
                ASTNode value = convertNode(valueObj);
                return new ReturnStmt(value);
            }
            case "comment" -> {
                GraceObject textObj = requestSync(node, "value", List.of());
                String text = GraceString.assertString(textObj).toString();
                return new Comment(text);
            }
            case "importStmt" -> {
                GraceObject sourceObj = requestSync(node, "source", List.of());
                String source = GraceString.assertString(sourceObj).toString();
                GraceObject bindingObj = requestSync(node, "binding", List.of());
                var binding = (IdentifierDeclaration)convertNode(bindingObj);
                return new ImportStmt(source, binding);
            }
            case "defDec" -> {
                GraceObject nameObj = requestSync(node, "name", List.of());
                String name = GraceString.assertString(nameObj).toString();
                GraceObject dtypeObj = requestSync(node, "decType", List.of());
                ASTNode dtype = convertNode(dtypeObj);
                GraceObject annotsObj = requestSync(node, "annotations", List.of());
                List<String> annots = convertStringList(annotsObj);
                GraceObject bodyObj = requestSync(node, "value", List.of());
                ASTNode body = convertNode(bodyObj);
                return new DefDec(name, dtype, annots, body);
            }
            case "varDec" -> {
                GraceObject nameObj = requestSync(node, "name", List.of());
                String name = GraceString.assertString(nameObj).toString();
                GraceObject dtypeObj = requestSync(node, "decType", List.of());
                ASTNode dtype = convertNode(dtypeObj);
                GraceObject annotsObj = requestSync(node, "annotations", List.of());
                List<ASTNode> annots = convertNodeList(annotsObj);
                GraceObject valueObj = requestSync(node, "value", List.of());
                ASTNode value = convertNode(valueObj);
                return new VarDec(name, dtype, annots, value);
            }
            case "block" -> {
                GraceObject paramsObj = requestSync(node, "parameters", List.of());
                List<ASTNode> params = convertNodeList(paramsObj);
                GraceObject bodyObj = requestSync(node, "body", List.of());
                List<ASTNode> body = convertNodeList(bodyObj);
                return new Block(params, body);
            }
            case "interpStr" -> {
                GraceObject preObj = requestSync(node, "value", List.of());
                String pre = GraceString.assertString(preObj).toString();
                GraceObject exprObj = requestSync(node, "expression", List.of());
                ASTNode expr = convertNode(exprObj);
                GraceObject postObj = requestSync(node, "next", List.of());
                ASTNode post = convertNode(postObj);
                return new InterpStr(pre, expr, post);
            }
            case "methDec" -> {
                GraceObject partsObj = requestSync(node, "parts", List.of());
                List<ASTNode> parts = convertNodeList(partsObj);
                GraceObject returnTypeObj = requestSync(node, "returnType", List.of());
                ASTNode returnType = convertNode(returnTypeObj);
                GraceObject annotsObj = requestSync(node, "annotations", List.of());
                List<ASTNode> annots = convertNodeList(annotsObj);
                GraceObject bodyObj = requestSync(node, "body", List.of());
                List<ASTNode> body = convertNodeList(bodyObj);
                return new MethodDecl(parts.stream().map(x -> (Part)x).toList(), returnType, annots, body);
            }
            case "nil" -> {
                // For optional values - lists are converted below
                return null;
            }
            case "cons" -> {
                // For optional values, not lists
                GraceObject headObj = requestSync(node, "head", List.of());
                ASTNode head = convertNode(headObj);
                return head;
            }
            default -> {
                throw new RuntimeException("Unknown AST node kind: " + nodeKind);
            }
        }
        // return null;
    }

    private List<ASTNode> convertNodeList(GraceObject listObj) {
        String listKind = GraceString.assertString(requestSync(listObj, "kind", List.of())).toString();
        if ("nil".equals(listKind)) {
            return List.of();
        } else if ("cons".equals(listKind)) {
            List<ASTNode> result = new java.util.ArrayList<>();
            GraceObject headObj = requestSync(listObj, "head", List.of());
            GraceObject tailObj = requestSync(listObj, "tail", List.of());
            while (true) {
                ASTNode headNode = convertNode(headObj);
                result.add(headNode);
                if ("nil".equals(GraceString.assertString(requestSync(tailObj, "kind", List.of())).toString()))
                    break;
                headObj = requestSync(tailObj, "head", List.of());
                tailObj = requestSync(tailObj, "tail", List.of());
            }
            return result;
        } else {
            throw new RuntimeException("Expected list object but got kind: " + listKind);
        }
    }

    private List<String> convertStringList(GraceObject listObj) {
        String listKind = GraceString.assertString(requestSync(listObj, "kind", List.of())).toString();
        if ("nil".equals(listKind)) {
            return List.of();
        } else if ("cons".equals(listKind)) {
            List<String> result = new java.util.ArrayList<>();
            GraceObject headObj = requestSync(listObj, "head", List.of());
            GraceObject tailObj = requestSync(listObj, "tail", List.of());
            while (true) {
                String headStr = GraceString.assertString(headObj).toString();
                result.add(headStr);
                if ("nil".equals(GraceString.assertString(requestSync(tailObj, "kind", List.of())).toString()))
                    break;
                headObj = requestSync(tailObj, "head", List.of());
                tailObj = requestSync(tailObj, "tail", List.of());
            }
            return result;
        } else {
            throw new RuntimeException("Expected list object but got kind: " + listKind);
        }
    }

    public GraceObject requestSync(GraceObject receiver, String methodName, List<GraceObject> args) {
        final GraceObject[] resultHolder = new GraceObject[1];
        final boolean[] completed = new boolean[] { false };

        var step = receiver.requestMethod(ctx, (value) -> {
            resultHolder[0] = value;
            completed[0] = true;
            return null;
        }, methodName, args);

        while (step != null && !completed[0]) {
            step = step.go();
        }

        return resultHolder[0];
    }
}
