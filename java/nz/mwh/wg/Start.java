package nz.mwh.wg;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import nz.mwh.wg.ast.ASTNode;

public class Start {
    public static void main(String[] args) {
        String filename = "test.grace";
        boolean printAST = false;
        for (String arg : args) {
            if (arg.equals("-p")) {
                printAST = true;
            } else {
                filename = arg;
                break;
            }
        }
        try {
            String source = Files.readString(Path.of(filename));
            ASTNode ast = Parser.parse(source);
            if (printAST) {
                System.out.println(ast);
            } else {
                Evaluator.evaluateProgram(ast);
            }
        } catch (IOException e) {
            throw new RuntimeException("Error reading file: " + filename);
        }
    }
}
