module Runtime where

import Ast
import Data.Map (insert, empty, Map, fromList)
import qualified Data.Map as Data.Map
import Data.IORef
import Data.Char
import Data.List (isSuffixOf, isPrefixOf)

data Context = Context {
    continuation :: GraceObject -> IO ()
    , self :: GraceObject
    , localScope :: GraceObject
    , returnCont :: GraceObject -> IO ()
}

withCont :: Context -> ( GraceObject -> IO () ) -> Context
withCont ctx cont = Context {
    continuation = cont,
    self = self ctx
    , localScope = localScope ctx
    , returnCont = returnCont ctx
}

withSelf :: Context -> GraceObject -> Context
withSelf ctx slf = Context {
    continuation = continuation ctx
    , self = slf
    , localScope = slf
    , returnCont = returnCont ctx
}

withScope :: Context -> GraceObject -> Context
withScope ctx scope = Context {
    continuation = continuation ctx
    , self = self ctx
    , localScope = scope
    , returnCont = returnCont ctx
}

withReturn :: Context -> (GraceObject -> IO ()) -> Context
withReturn ctx cont = Context {
    continuation = continuation ctx
    , self = self ctx
    , localScope = localScope ctx
    , returnCont = cont
}


data GraceObject = BaseObject GraceObject (Map String (Context -> [GraceObject] -> IO ()))
                   | LocalScope GraceObject (Map String (IORef GraceObject))
                   | GraceNumber Float
                   | GraceString String
                   | GraceBool Bool
                   | GraceBlock [String] (Context -> IO ()) [String] Context
                   | GraceDone
                   | GraceErrorObject String

instance Show GraceObject where
    show (GraceNumber f) = show f
    show (GraceString s) = s
    show (GraceBool b) = show b
    show (LocalScope _ m) = "LocalScope(" ++ (show $ Data.Map.keys m) ++ ")"
    show GraceDone = "Done"
    show (GraceErrorObject s) = "Error: " ++ s
    show (GraceBlock params _ _ _) = "Block" ++ (show params)
    show (BaseObject _ m) = "BaseObject(" ++ (show $ Data.Map.keys m) ++ ")"

partsToName :: [Part] -> String
partsToName [] = ""
partsToName [Part n p] = n ++ "(" ++ (show $ length p) ++ ")"
partsToName ((Part n p):t) = n ++ "(" ++ (show $ length p) ++ ")" ++ partsToName t

getMethod :: String -> GraceObject -> (Context -> [GraceObject] -> IO ())
getMethod n (GraceNumber f) =
    case n of
        "+(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceNumber (f + other)
        "*(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceNumber (f * other)
        "-(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceNumber (f - other)
        "/(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceNumber (f / other)
        "==(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceBool (f == other)
        "!=(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceBool (f /= other)
        ">(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceBool (f > other)
        "<(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceBool (f < other)
        ">=(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceBool (f >= other)
        "<=(1)" -> \ctx [GraceNumber other] -> continuation ctx $ GraceBool (f <= other)
        "asString(0)" -> \ctx [] ->
            if f == fromInteger (round f)
                then continuation ctx $ GraceString (show (round f))
                else continuation ctx $ GraceString (show f)

getMethod n (GraceString s) =
    case n of
        "asString(0)" -> \ctx [] -> continuation ctx $ GraceString s
        "size(0)" -> \ctx [] -> continuation ctx $ GraceNumber (fromIntegral $ length s)
        "at(1)" -> \ctx [GraceNumber i] ->
            if i > 0 && i <= fromIntegral (length s)
                then continuation ctx $ GraceString [s !! ((floor i) - 1)]
                else continuation ctx $ GraceErrorObject $ "Index out of bounds: " ++ show i ++ " in " ++ show s
        "firstCP(0)" -> \ctx [] ->
            continuation ctx $ GraceNumber (fromIntegral $ ord $ s !! 0)
        "firstCodepoint(0)" -> \ctx [] ->
            continuation ctx $ GraceNumber (fromIntegral $ ord $ s !! 0)
        "==(1)" -> \ctx [GraceString other] -> continuation ctx $ GraceBool (s == other)
        "!=(1)" -> \ctx [GraceString other] -> continuation ctx $ GraceBool (s /= other)
        ">=(1)" -> \ctx [GraceString other] -> continuation ctx $ GraceBool (s >= other)
        "<=(1)" -> \ctx [GraceString other] -> continuation ctx $ GraceBool (s <= other)
        ">(1)" -> \ctx [GraceString other] -> continuation ctx $ GraceBool (s > other)
        "<(1)" -> \ctx [GraceString other] -> continuation ctx $ GraceBool (s < other)
        "++(1)" -> \ctx [o] ->
            case o of
                GraceString other -> continuation ctx $ GraceString (s ++ other)
                o ->
                    let asString = getMethod "asString(0)" o
                    in
                        asString (withCont ctx (\o' ->
                            case o' of
                                GraceString other -> continuation ctx $ GraceString (s ++ other)
                                _ -> continuation ctx $ GraceErrorObject $ "Illegal string concatenation: " ++ show s ++ " ++ " ++ show o
                        ) ) []
        "replace(1)with(1)" -> \ctx [GraceString old, GraceString new] ->
            continuation ctx $ GraceString (replace s old new)
        "substringFrom(1)to(1)" -> \ctx [GraceNumber start, GraceNumber end] ->
            if start > 0 && end >= start && end <= fromIntegral (length s)
                then continuation ctx $ GraceString (take (floor end - floor start + 1) (drop (floor start - 1) s))
                else if end < start then continuation ctx $ GraceString ""
                else continuation ctx $ GraceErrorObject $ "Index out of bounds: " ++ show start ++ " to " ++ show end ++ " in " ++ show s
        _ -> \ctx _ ->
            do
                putStrLn $ "String method not found: " ++ n
                return ()

getMethod n (GraceBool b) =
    case n of
        "asString(0)" -> \ctx [] -> continuation ctx $ GraceString (show b)
        "==(1)" -> \ctx [GraceBool other] -> continuation ctx $ GraceBool (b == other)
        "!=(1)" -> \ctx [GraceBool other] -> continuation ctx $ GraceBool (b /= other)
        "&&(1)" -> \ctx [GraceBool other] -> continuation ctx $ GraceBool (b && other)
        "||(1)" -> \ctx [GraceBool other] -> continuation ctx $ GraceBool (b || other)
        "prefix!(0)" -> \ctx _ -> continuation ctx $ GraceBool (not b)
        "not(0)" -> \ctx _ -> continuation ctx $ GraceBool (not b)
        _ -> \ctx _ ->
            do
                putStrLn $ "Boolean method not found: " ++ n
                return ()

getMethod n (BaseObject _ meths) =
    case Data.Map.lookup n meths of
        Just m -> m
        Nothing -> \ctx args ->
            do
                putStrLn $ "Method not found: " ++ n
                return ()

getMethod n (LocalScope _ meths) =
    case Data.Map.lookup (stripSuffix "(0)" n) meths of
        Just m -> \ctx args ->
            do
                val <- readIORef m
                (continuation ctx) val
        Nothing -> case Data.Map.lookup (stripSuffix ":=(1)" n) meths of
            Just m -> \ctx [val] ->
                do
                    writeIORef m val
                    (continuation ctx) GraceDone
            Nothing ->
                case Data.Map.lookup (stripPrefix "def " n) meths of
                    Just m -> \ctx [val] ->
                        do
                            writeIORef m val
                            (continuation ctx) GraceDone
                    Nothing ->
                        \ctx args ->
                            do
                                putStrLn $ "Scope method not found: " ++ n ++ "; have " ++ (show $ Data.Map.keys meths)
                                return ()

getMethod n (GraceBlock params body vars creationContext) =
    let applyName = "apply(" ++ (show $ length params) ++ ")"
    in \ctx args ->
            do
                argMap <- makeScopeMap params args vars
                let scope = LocalScope (localScope creationContext) argMap
                let selfCtx = withScope (withCont creationContext $ continuation ctx) scope
                body selfCtx


getMethod n o =
    \ctx _ ->
        do
            putStrLn $ "Method not found: " ++ n
            putStrLn $ "In object: " ++ (show o)

replace :: Eq a => [a] -> [a] -> [a] -> [a]
replace [] _ _ = []
replace s find repl =
    if isPrefixOf find s
        then repl ++ (replace (drop (length find) s) find repl)
        else
            case s of
                (h:t) -> h : (replace t find repl)

findReceiver :: String -> Context -> GraceObject
findReceiver name ctx =
    findReceiver' name (localScope ctx) (localScope ctx)

findReceiver' :: String -> GraceObject -> GraceObject -> GraceObject
findReceiver' name o@(BaseObject outer meths) start =
    case Data.Map.lookup name meths of
        Just m -> o
        Nothing -> findReceiver' name outer start

findReceiver' name o@(LocalScope outer vars) start =
    if isSuffixOf "(0)" name
        then
            case Data.Map.lookup (stripSuffix "(0)" name) vars of
                Just m -> o
                Nothing -> findReceiver' name outer start
        else if isSuffixOf ":=(1)" name
            then
                case Data.Map.lookup (stripSuffix ":=(1)" name) vars of
                    Just m -> o
                    Nothing -> findReceiver' name outer start
            else if isPrefixOf "def " name
                then
                    case Data.Map.lookup (stripPrefix "def " name) vars of
                        Just m -> o
                        Nothing -> findReceiver' name outer start
                else
                    findReceiver' name outer start

findReceiver' name o start = GraceErrorObject $ "Receiver not found: " ++ name ++ " from " ++ (show start)

stripSuffix :: String -> String -> String
stripSuffix s str =
    if isSuffixOf s str
        then take (length str - length s) str
        else str

stripPrefix :: String -> String -> String
stripPrefix s str =
    if isPrefixOf s str
        then drop (length s) str
        else str




--              params       args             vars
makeScopeMap :: [String] -> [GraceObject] -> [String] -> IO (Map String (IORef GraceObject))
makeScopeMap [] [] [] = return empty
makeScopeMap (name:ps) (a:as) vars =
    do
        cell <- newIORef a
        restMap <- makeScopeMap ps as vars
        return $ insert name cell restMap
makeScopeMap [] [] (name:t) =
    do
        cell <- newIORef GraceDone
        restMap <- makeScopeMap [] [] t
        writeIORef cell GraceDone
        return $ insert name cell restMap

evalArgSeq :: Context -> [Context -> IO ()] -> ([GraceObject] -> IO ()) -> IO ()
evalArgSeq _   []    cont = cont []
evalArgSeq ctx (a:as) cont =
    a $ withCont ctx (\arg ->
        evalArgSeq ctx as (\args ->
            cont (arg:args))
    )



gracePrelude = BaseObject GraceDone $ fromList [
    ("print(1)", \ctx [arg] ->
        do
            putStrLn $ "Grace print: " ++ (show arg)
            (continuation ctx) GraceDone
    )
    , ("if(1)then(1)", \ctx [cond, thenBlock] ->
        do
            let apply = getMethod "apply(0)" thenBlock
            case cond of
                GraceBool True ->
                    apply ctx []
                GraceBool False ->
                    (continuation ctx) GraceDone
                _ ->
                    (continuation ctx) $ GraceErrorObject $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)else(1)", \ctx [cond, thenBlock, elseBlock] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElse = getMethod "apply(0)" elseBlock
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    applyElse ctx []
                _ ->
                    (continuation ctx) $ GraceErrorObject $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)elseif(1)then(1)", \ctx [cond, thenBlock, elseifCondBlock, elseifThenBlock] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElseIfThen = getMethod "apply(0)" elseifThenBlock
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    let applyElseIfCond = getMethod "apply(0)" elseifCondBlock
                    in
                        applyElseIfCond (withCont ctx (\cond ->
                            case cond of
                                GraceBool True ->
                                    applyElseIfThen ctx []
                                GraceBool False ->
                                    (continuation ctx) GraceDone
                                _ ->
                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                        ) ) []
                _ ->
                    putStrLn $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)elseif(1)then(1)else(1)", \ctx [cond, thenBlock, elseifCondBlock, elseifThenBlock, elseBlock] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElseIfThen = getMethod "apply(0)" elseifThenBlock
            let applyElse = getMethod "apply(0)" elseBlock
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    let applyElseIfCond = getMethod "apply(0)" elseifCondBlock
                    in
                        applyElseIfCond (withCont ctx (\cond ->
                            case cond of
                                GraceBool True ->
                                    applyElseIfThen ctx []
                                GraceBool False ->
                                    applyElse ctx []
                                _ ->
                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                        ) ) []
                _ ->
                    putStrLn $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)elseif(1)then(1)elseif(1)then(1)", \ctx [cond, thenBlock, elseifCondBlock, elseifThenBlock, elseifCondBlock2, elseifThenBlock2] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElseIfThen = getMethod "apply(0)" elseifThenBlock
            let applyElseIfThen2 = getMethod "apply(0)" elseifThenBlock2
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    let applyElseIfCond = getMethod "apply(0)" elseifCondBlock
                    in
                        applyElseIfCond (withCont ctx (\cond ->
                            case cond of
                                GraceBool True ->
                                    applyElseIfThen ctx []
                                GraceBool False ->
                                    let applyElseIfCond2 = getMethod "apply(0)" elseifCondBlock2
                                    in
                                        applyElseIfCond2 (withCont ctx (\cond ->
                                            case cond of
                                                GraceBool True ->
                                                    applyElseIfThen2 ctx []
                                                GraceBool False ->
                                                    (continuation ctx) GraceDone
                                                _ ->
                                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                                        ) ) []
                                _ ->
                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                        ) ) []
                _ ->
                    putStrLn $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", \ctx [cond, thenBlock, elseifCondBlock, elseifThenBlock, elseifCondBlock2, elseifThenBlock2, elseBlock] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElseIfThen = getMethod "apply(0)" elseifThenBlock
            let applyElseIfThen2 = getMethod "apply(0)" elseifThenBlock2
            let applyElse = getMethod "apply(0)" elseBlock
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    let applyElseIfCond = getMethod "apply(0)" elseifCondBlock
                    in
                        applyElseIfCond (withCont ctx (\cond ->
                            case cond of
                                GraceBool True ->
                                    applyElseIfThen ctx []
                                GraceBool False ->
                                    let applyElseIfCond2 = getMethod "apply(0)" elseifCondBlock2
                                    in
                                        applyElseIfCond2 (withCont ctx (\cond ->
                                            case cond of
                                                GraceBool True ->
                                                    applyElseIfThen2 ctx []
                                                GraceBool False ->
                                                    applyElse ctx []
                                                _ ->
                                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                                        ) ) []
                                _ ->
                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                        ) ) []
                _ ->
                    putStrLn $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)", \ctx [cond, thenBlock, elseifCondBlock, elseifThenBlock, elseifCondBlock2, elseifThenBlock2, elseifCondBlock3, elseifThenBlock3] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElseIfThen = getMethod "apply(0)" elseifThenBlock
            let applyElseIfThen2 = getMethod "apply(0)" elseifThenBlock2
            let applyElseIfThen3 = getMethod "apply(0)" elseifThenBlock3
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    let applyElseIfCond = getMethod "apply(0)" elseifCondBlock
                    in
                        applyElseIfCond (withCont ctx (\cond ->
                            case cond of
                                GraceBool True ->
                                    applyElseIfThen ctx []
                                GraceBool False ->
                                    let applyElseIfCond2 = getMethod "apply(0)" elseifCondBlock2
                                    in
                                        applyElseIfCond2 (withCont ctx (\cond ->
                                            case cond of
                                                GraceBool True ->
                                                    applyElseIfThen2 ctx []
                                                GraceBool False ->
                                                    let applyElseIfCond3 = getMethod "apply(0)" elseifCondBlock3
                                                    in
                                                        applyElseIfCond3 (withCont ctx (\cond ->
                                                            case cond of
                                                                GraceBool True ->
                                                                    applyElseIfThen3 ctx []
                                                                GraceBool False ->
                                                                    (continuation ctx) GraceDone
                                                                _ ->
                                                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                                                        ) ) []
                                                _ ->
                                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                                        ) ) []
                                _ ->
                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                        ) ) []
                _ ->
                    putStrLn $ "Illegal if condition: " ++ (show cond)
    )
    , ("if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", \ctx [cond, thenBlock, elseifCondBlock, elseifThenBlock, elseifCondBlock2, elseifThenBlock2, elseifCondBlock3, elseifThenBlock3, elseBlock] ->
        do
            let applyThen = getMethod "apply(0)" thenBlock
            let applyElseIfThen = getMethod "apply(0)" elseifThenBlock
            let applyElseIfThen2 = getMethod "apply(0)" elseifThenBlock2
            let applyElseIfThen3 = getMethod "apply(0)" elseifThenBlock3
            let applyElse = getMethod "apply(0)" elseBlock
            case cond of
                GraceBool True ->
                    applyThen ctx []
                GraceBool False ->
                    let applyElseIfCond = getMethod "apply(0)" elseifCondBlock
                    in
                        applyElseIfCond (withCont ctx (\cond ->
                            case cond of
                                GraceBool True ->
                                    applyElseIfThen ctx []
                                GraceBool False ->
                                    let applyElseIfCond2 = getMethod "apply(0)" elseifCondBlock2
                                    in
                                        applyElseIfCond2 (withCont ctx (\cond ->
                                            case cond of
                                                GraceBool True ->
                                                    applyElseIfThen2 ctx []
                                                GraceBool False ->
                                                    let applyElseIfCond3 = getMethod "apply(0)" elseifCondBlock3
                                                    in
                                                        applyElseIfCond3 (withCont ctx (\cond ->
                                                            case cond of
                                                                GraceBool True ->
                                                                    applyElseIfThen3 ctx []
                                                                GraceBool False ->
                                                                    applyElse ctx []
                                                                _ ->
                                                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                                                        ) ) []
                                                _ ->
                                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                                        ) ) []
                                _ ->
                                    (continuation ctx) $ GraceErrorObject $ "Illegal elseif condition: " ++ (show cond)
                        ) ) []
                _ ->
                    putStrLn $ "Illegal if condition: " ++ (show cond)
    )
    , ("while(1)do(1)", \ctx [cond, body] ->
        do
            let test = getMethod "apply(0)" cond
            let apply = getMethod "apply(0)" body
            let loop = \result ->
                    case result of
                        GraceBool True ->
                            apply (withCont ctx (\_ -> test (withCont ctx loop) [] ) ) []
                        GraceBool False ->
                            (continuation ctx) GraceDone
                        _ ->
                            putStrLn $ "Illegal while condition: " ++ (show result)
            test (withCont ctx loop) []
    )
    , ("true(0)", \ctx [] ->
        do
            (continuation ctx) $ GraceBool True
    )
    , ("false(0)", \ctx [] ->
        do
            (continuation ctx) $ GraceBool False
    )
    ]


printContext = Context {
    continuation = \obj -> do
        putStrLn $ show obj
        return ()
    , self = gracePrelude
    , localScope = gracePrelude
    , returnCont = \obj -> do
        putStrLn $ "Illegal top-level return of " ++ (show obj)
}

dropContext = Context {
    continuation = \obj -> do
        return ()
    , self = gracePrelude
    , localScope = gracePrelude
    , returnCont = \obj -> do
        putStrLn $ "Illegal top-level return of " ++ (show obj)
}

resultContext func = Context {
    continuation = func
    , self = gracePrelude
    , localScope = gracePrelude
    , returnCont = \obj -> do
        putStrLn $ "Illegal top-level return of " ++ (show obj)
}
