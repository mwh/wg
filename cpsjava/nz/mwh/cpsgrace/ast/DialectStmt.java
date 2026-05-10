package nz.mwh.cpsgrace.ast;

import java.util.List;
import java.util.Set;
import java.util.HashSet;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.Start;
import nz.mwh.cpsgrace.TheProgram;
import nz.mwh.cpsgrace.objects.UserObject;

public class DialectStmt extends ASTNode {
    private String moduleName;

    public DialectStmt(String moduleName) {
        this.moduleName = moduleName;
        ImportStmt.IMPORTED_MODULES.add(moduleName);
    }

    public String getModuleName() {
        return moduleName;
    }

    public CPS toCPS() {
        ASTNode moduleNode = TheProgram.getModuleAST(moduleName);
        CPS moduleCPS = moduleNode.toCPS();
        return (ctx, cont) -> {
            GraceObject scope = ctx.getScope();
            UserObject uo = (UserObject)scope;
            GraceObject cached = TheProgram.importedModules.get(moduleName);
            if (cached != null) {
                uo.setDialect(cached);
                return cont.returning(ctx, GraceObject.DONE);
            }
            return moduleCPS.run(ctx.withScope(Start.prelude), (moduleObj) -> {
                ((UserObject)moduleObj).setDebugLabel("imported module " + moduleName);
                TheProgram.importedModules.put(moduleName, moduleObj);
                uo.setDialect(moduleObj);
                return cont.returning(ctx, GraceObject.DONE);
            });
        };
    }

}

