package nz.mwh.cpsgrace;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

import nz.mwh.cpsgrace.ast.ASTNode;
import nz.mwh.cpsgrace.baked.ASTData;
import nz.mwh.cpsgrace.baked.ParserData;

public class TheProgram extends nz.mwh.cpsgrace.ast.ASTEncoding {
    @SuppressWarnings("unchecked")
    public static ASTNode program = objCons(cons(comment(" Algol Bulletin, issue 17, Jul. 1964. Letter by Donald Knuth, p7."), cons(methDec(one(part("A", cons(identifierDeclaration("k", nil), cons(identifierDeclaration("x1", nil), cons(identifierDeclaration("x2", nil), cons(identifierDeclaration("x3", nil), cons(identifierDeclaration("x4", nil), one(identifierDeclaration("x5", nil))))))))), nil, nil, cons(varDec("k'", nil, nil, one(lexReq(one(part("k", nil))))), cons(varDec("aRet", nil, nil, nil), cons(defDec("B", nil, nil, block(nil, cons(varDec("bRet", nil, nil, nil), cons(assn(lexReq(one(part("k'", nil))), dotReq(lexReq(one(part("k'", nil))), one(part("-", one(numLit(1)))))), cons(assn(lexReq(one(part("aRet", nil))), lexReq(one(part("A", cons(lexReq(one(part("k'", nil))), cons(lexReq(one(part("B", nil))), cons(lexReq(one(part("x1", nil))), cons(lexReq(one(part("x2", nil))), cons(lexReq(one(part("x3", nil))), one(lexReq(one(part("x4", nil))))))))))))), cons(assn(lexReq(one(part("bRet", nil))), lexReq(one(part("aRet", nil)))), one(lexReq(one(part("bRet", nil)))))))))), cons(lexReq(cons(part("if", one(dotReq(lexReq(one(part("k'", nil))), one(part("<=", one(numLit(0))))))), cons(part("then", one(block(nil, one(assn(lexReq(one(part("aRet", nil))), dotReq(dotReq(lexReq(one(part("x4", nil))), one(part("apply", nil))), one(part("+", one(dotReq(lexReq(one(part("x5", nil))), one(part("apply", nil)))))))))))), one(part("else", one(block(nil, one(dotReq(lexReq(one(part("B", nil))), one(part("apply", nil))))))))))), one(lexReq(one(part("aRet", nil))))))))), one(lexReq(one(part("print", one(lexReq(one(part("A", cons(numLit(15), cons(block(nil, one(numLit(1))), cons(block(nil, one(dotReq(numLit(1), one(part("prefix-", nil))))), cons(block(nil, one(dotReq(numLit(1), one(part("prefix-", nil))))), cons(block(nil, one(numLit(1))), one(block(nil, one(numLit(0))))))))))))))))))), nil);

    public static Map<String, Class<?>> importableModules = new HashMap<>(Map.of(
        "ast", ASTData.class
        , "parser", ParserData.class
    ));

    public static Map<String, GraceObject> importedModules = new HashMap<>();

    public static Map<String, ASTNode> moduleASTs = new HashMap<>();

    public static ASTNode getModuleAST(String name) {
        if (moduleASTs.containsKey(name)) {
            return moduleASTs.get(name);
        } else if (importableModules.containsKey(name)) {
            try {
                Class<?> cls = importableModules.get(name);
                Method method = cls.getMethod("module");
                ASTNode node = (ASTNode) method.invoke(null);
                moduleASTs.put(name, node);
                return node;
            } catch (NoSuchMethodException | IllegalAccessException | InvocationTargetException e) {
                e.printStackTrace();
                throw new RuntimeException("failed to get module AST for " + name);
            }
        }
        return null;
    }

}