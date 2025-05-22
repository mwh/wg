import Program
import Ast (prettyPrint)
import CPS (toFunc)
import Runtime

main =
    do
        let func = toFunc program
        func dropContext

dump =
    putStrLn $ prettyPrint program


-- When program = the parser, this will allow running the parser on
-- an arbitrary string from the REPL.
runParse s =
    do
        let func = toFunc program
        func $ resultContext $ \val ->
            let parse = getMethod "parse(1)" val
                string = GraceString s
            in
                parse (resultContext (\result ->
                    do
                        let asString = getMethod "asString(0)" result
                        asString printContext []
                        --putStrLn $ "Result: " ++ (show result)
                        --return ()
                    )
                ) [string]

parseExecute s =
    do
        let func = toFunc program
        func $ resultContext $ \val ->
            let parse = getMethod "parse(1)" val
                string = GraceString s
            in
                parse (resultContext (\result ->
                    do
                        case result of
                            GraceAstObject n -> do
                                toFunc n $ dropContext
                            _ -> putStrLn $ "Parse result was not an AST object: " ++ (show result)
                    )
                ) [string]
