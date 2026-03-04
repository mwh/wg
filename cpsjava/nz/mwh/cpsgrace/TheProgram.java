package nz.mwh.cpsgrace;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.HashMap;
import java.util.Map;

import nz.mwh.cpsgrace.ast.ASTNode;
import nz.mwh.cpsgrace.baked.ASTData;
import nz.mwh.cpsgrace.baked.ParserData;
import nz.mwh.cpsgrace.baked.LexerData;

public class TheProgram extends nz.mwh.cpsgrace.ast.ConciseEncoding {
    @SuppressWarnings("unchecked")
    public static ASTNode program = o0C(c0N(m0D(o1N(p0T("foo",nil,c2N(i0D("X",nil),i0D("Y",nil)))),o1N(l0R("X(0)",nil,nil)),nil,o1N(n0M(1))),c0N(l0R("print(1)",o1N(l0R("foo(0)",nil,c2N(l0R("Number(0)",nil,nil),l0R("String(0)",nil,nil)))),nil),c0N(c0M(s4F(" type List[[T]] = interface ",c9L,"")),c2N(c0M("     map[[S]](b : Block[[T,S]]) -> List[[S]]"),c0M(" }"))))),nil);

    public static Map<String, Class<?>> importableModules = new HashMap<>(Map.of(
        "ast", ASTData.class
        , "parser", ParserData.class
        , "lexer", LexerData.class
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