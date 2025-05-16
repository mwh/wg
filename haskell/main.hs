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
