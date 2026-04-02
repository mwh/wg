package nz.mwh.cpsgrace.ast;

import java.util.List;
import java.util.Set;
import java.util.HashSet;

import nz.mwh.cpsgrace.CPS;
import nz.mwh.cpsgrace.GraceObject;
import nz.mwh.cpsgrace.Start;
import nz.mwh.cpsgrace.TheProgram;
import nz.mwh.cpsgrace.objects.UserObject;

public class ImportStmt extends ASTNode {
    private String moduleName;
    private IdentifierDeclaration asName;

    public static Set<String> IMPORTED_MODULES = new HashSet<>();

    public ImportStmt(String moduleName, IdentifierDeclaration asName) {
        this.moduleName = moduleName;
        this.asName = asName;
        IMPORTED_MODULES.add(moduleName);
    }

    public String getModuleName() {
        return moduleName;
    }

    public IdentifierDeclaration getAsName() {
        return asName;
    }

    public CPS toCPS() {
        ASTNode moduleNode = TheProgram.getModuleAST(moduleName);
        CPS moduleCPS = moduleNode.toCPS();
        return (ctx, cont) -> {
            GraceObject receiver = ctx.findReceiver(asName.getName() + " =(1)");
            GraceObject cached = TheProgram.importedModules.get(moduleName);
            if (cached != null) {
                return receiver.requestMethod(ctx, cont, asName.getName() + " =(1)", List.of(cached));
            }
            return moduleCPS.run(ctx.withScope(Start.prelude), (moduleObj) -> {
                ((UserObject)moduleObj).setDebugLabel("imported module " + moduleName);
                TheProgram.importedModules.put(moduleName, moduleObj);
                return receiver.requestMethod(ctx, cont, asName.getName() + " =(1)", List.of(moduleObj));
            });
        };
    }

}

