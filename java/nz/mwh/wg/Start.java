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

// import nz.mwh.wg.css.Parser;
import nz.mwh.wg.css.Rule;

public class Start {
    public static void main(String[] args) {
        Rule r = null; //new nz.mwh.wg.css.Parser("method object var {}").parseRule();
        String filename = "test.grace";
        boolean printAST = false;
        String updateFile = null;
        boolean inlineImports = false;
        String ruleString = null;
        for (String arg : args) {
            if (arg.equals("-p")) {
                printAST = true;
            } else if (arg.equals("-u")) {
                updateFile = "";
            } else if (updateFile != null && updateFile.isEmpty()) {
                updateFile = arg;
            } else if (arg.equals("-i")) {
                inlineImports = true;
            } else if (arg.equals("--rule")) {
                ruleString = "";
            } else if (ruleString != null && ruleString.isEmpty()) {
                ruleString = arg;
            } else {
                filename = arg;
                break;
            }
        }
        if (ruleString != null) {
            r = new nz.mwh.wg.css.Parser(ruleString).parseRule();
            System.out.println(r + "\n--------------------");
        }
        try {
            String source = Files.readString(Path.of(filename));
            ASTNode ast = Parser.parse(source);
            if (inlineImports) {
                inlineImports((ObjectConstructor) ast);
            }
            if (printAST) {
                System.out.println(ast);
            } else if (updateFile != null) {
                updateFile(ast, updateFile);
            } else {
                if (r != null) {
                    Evaluator.addRule(r);
                }
                Evaluator.evaluateProgram(ast);
            }
        } catch (IOException e) {
            throw new RuntimeException("Error reading file: " + filename);
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
