import Program
import Ast (prettyPrint)
import CPS (toFunc)
import Runtime
import System.Environment (getArgs)

main =
    do
        args <- getArgs
        case args of
            [] -> do
                let func = toFunc program
                func dropContext
            ["run", path] -> do
                contents <- readFile path
                parseExecute contents
            ["eval", code] ->
                parseExecute code
            ["dump"] ->
                dump
            ["dumpFile", path] -> do
                contents <- readFile path
                processAST contents (\val -> case val of GraceAstObject n -> putStrLn $ prettyPrint n)
            ["dump", code] ->
                processAST code (\val -> case val of GraceAstObject n -> putStrLn $ prettyPrint n)
            _ -> do
                putStrLn "Invalid arguments. Use no arguments to run hard-coded program, or if parser is compiled in, 'run <path>' to run a file, or 'eval <code>' to evaluate a string."

dump =
    putStrLn $ prettyPrint program


-- When program = the parser, this will allow running the parser on
-- arbitrary code, and then doing something with the result.
processAST code using =
    do
        let func = toFunc program
        func $ resultContext $ \val ->
            let parse = getMethod "parse(1)" val
                string = GraceString code
            in
                parse (resultContext using) [string]

runParse s = processAST s $ (\result ->
                    do
                        let asString = getMethod "asString(0)" result
                        asString printContext []
                    )

parseExecute s = processAST s $ (\result ->
                    do
                        case result of
                            GraceAstObject n -> do
                                toFunc n $ dropContext
                            _ -> putStrLn $ "Parse result was not an AST object: " ++ (show result)
                    )
