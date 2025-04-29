package nz.mwh.wg;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Pattern;

import nz.mwh.wg.ast.ASTNode;
import nz.mwh.wg.ast.Cons;
import nz.mwh.wg.ast.DefDecl;
import nz.mwh.wg.ast.ImportStmt;
import nz.mwh.wg.ast.ObjectConstructor;
import nz.mwh.wg.runtime.GraceException;

public class Start {
    public static void main(String[] args) {
        String filename = "test.grace";
        boolean printAST = false;
        String updateFile = null;
        boolean inlineImports = false;

        String isoWhen = null;
        String isoMove = null;
        String localWhen = null;
        String capabilityWhere = null;
        String capabilityChange = null;
        String immWhen = null;

        for (String arg : args) {
            if (arg.equals("-p")) {
                printAST = true;
            } else if (arg.equals("-u")) {
                updateFile = "";
            } else if (updateFile != null && updateFile.isEmpty()) {
                updateFile = arg;
            } else if (arg.equals("-i")) {
                inlineImports = true;
            } else if (arg.equals("--iso-when")) {
                isoWhen = "";
            } else if (isoWhen != null && isoWhen.isEmpty()) {
                isoWhen = arg;
            } else if (arg.equals("--iso-move")) {
                isoMove = "";
            } else if (isoMove != null && isoMove.isEmpty()) {
                isoMove = arg;
            } else if (arg.equals("--local-when")) {
                localWhen = "";
            } else if (localWhen != null && localWhen.isEmpty()) {
                localWhen = arg;
            } else if (arg.equals("--capability-where")) {
                capabilityWhere = "";
            } else if (capabilityWhere != null && capabilityWhere.isEmpty()) {
                capabilityWhere = arg;
            } else if (arg.equals("--capability-change")) {
                capabilityChange = "";
            } else if (capabilityChange != null && capabilityChange.isEmpty()) {
                capabilityChange = arg;
            } else if (arg.equals("--imm-when")) {
                immWhen = "";
            } else if (immWhen != null && immWhen.isEmpty()) {
                immWhen = arg;
            } else {
                filename = arg;
                break;
            }
        }
        if (isoWhen != null) {
            Dala.setIsoWhen(isoWhen);
        }
        if (isoMove != null) {
            Dala.setIsoMove(isoMove);
        }
        if (localWhen != null) {
            Dala.setLocalWhen(localWhen);
        }
        if (capabilityWhere != null) {
            Dala.setCapabilityWhere(capabilityWhere);
        }
        if (capabilityChange != null) {
            Dala.setCapabilityChange(capabilityChange);
        }
        if (immWhen != null) {
            Dala.setImmWhen(immWhen);
        }
        try {
            String source = Files.readString(Path.of(filename));
            ASTNode ast = Parser.parse(Path.of(filename).getFileName().toString(), source);
            if (inlineImports) {
                inlineImports((ObjectConstructor) ast);
            }
            if (printAST) {
                System.out.println(ast);
            } else if (updateFile != null) {
                updateFile(ast, updateFile);
            } else {
                try {
                    Evaluator.evaluateProgram(ast);
                } catch (GraceException e) {
                    System.err.println("Crash during evaluation: " + e);
                    System.err.println("From call to:");
                    for (String entry : e.getCallStack()) {
                        System.err.println("  " + entry);
                    }
                    System.err.println("Crash during evaluation: " + e);
                }
            }
        } catch (GraceException e) {
            var kind = e.getKind();
            if (kind != null && "ParseError".equals(kind.getName())) {
                System.err.println(e.getMessage());
                System.exit(1);
            }
            System.err.println("Grace crash during parsing: " + e);
            System.err.println("At:");
            for (String entry : e.getCallStack()) {
                System.err.println("  " + entry);
            }
            System.err.println("Grace crash during parsing: " + e);
        } catch (IOException e) {
            throw new RuntimeException("Error reading file: " + filename);
        } catch (RuntimeException e) {
            System.err.println("Java crash: " + e.getMessage());
            for (StackTraceElement element : e.getStackTrace()) {
                System.err.println("  " + element);
            }
            System.err.println("Java crash: " + e.getMessage());
        }
    }

    private static void inlineImports(ObjectConstructor ast) throws IOException {
        List<ASTNode> body = ast.getBody();
        for (int i = 0; i < body.size(); i++) {
            ASTNode node = body.get(i);
            if (node instanceof ImportStmt) {
                ImportStmt importStmt = (ImportStmt) node;
                String filename = importStmt.getSource() + ".grace";
                String source = Files.readString(Path.of(filename));
                ObjectConstructor imported = (ObjectConstructor) Parser.parse(source);
                inlineImports(imported);
                DefDecl def = new DefDecl(importStmt.getName(), null, Cons.nil(), imported);
                body.set(i, def);
            }
        }
    }

    private static void updateFile(ASTNode ast, String filename) {
        try {
            List<String> source = Files.readAllLines(Path.of(filename));
            List<String> newSource = new ArrayList<>();
            Pattern pattern = Pattern.compile("^(|.*\\s)program\\s*=");
            System.out.println(pattern);
            boolean foundMatch = false;
            for (String line : source) {
                if (pattern.matcher(line).lookingAt()) {
                    int pos = line.indexOf("program");
                    String newLine = line.substring(0, pos + 7) + " = " + ast.toString() + (line.endsWith(";") ? ";" : "");
                    newSource.add(newLine);
                    System.out.println("Found program line");
                    foundMatch = true;
                } else {
                    newSource.add(line);
                }
            }
            if (!foundMatch) {
                throw new RuntimeException("No program line found in file: " + filename);
            }
            Files.writeString(Path.of(filename), String.join("\n", newSource));
        } catch (IOException e) {
            throw new RuntimeException("Error writing file: " + filename);
        }
    }
}
