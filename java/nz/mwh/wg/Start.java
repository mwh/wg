package nz.mwh.wg;

import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

import nz.mwh.wg.ast.ASTNode;

public class Start {
    public static void main(String[] args) {
        try {
            String source = Files.readString(Path.of(args.length > 0 ? args[0] : "test.grace"));
            ASTNode ast = Parser.parse(source);
            Evaluator.evaluateProgram(ast);
        } catch (IOException e) {
            throw new RuntimeException("Error reading file: test.grace");
        }
    }
}
