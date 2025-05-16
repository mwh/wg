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
    show o = "GraceObject"

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
        "asString(0)" -> \ctx [] -> continuation ctx $ GraceString (show f)

getMethod n (GraceString s) =
    case n of
        "asString(0)" -> \ctx [] -> continuation ctx $ GraceString s
        "size(0)" -> \ctx [] -> continuation ctx $ GraceNumber (fromIntegral $ length s)
        "at(1)" -> \ctx [GraceNumber i] ->
            if i >= 0 && i < fromIntegral (length s)
                then continuation ctx $ GraceString [s !! (floor i)]
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
                                putStrLn $ "Scope method not found: " ++ n
                                return ()

getMethod n (GraceBlock params body vars creationContext) =
    let applyName = "apply(" ++ (show $ length params) ++ ")"
    in \ctx args ->
            do
                argMap <- makeScopeMap params args vars
                let scope = LocalScope (self creationContext) argMap
                let selfCtx = withScope (withCont creationContext $ continuation ctx) scope
                body selfCtx


getMethod n o =
    \ctx _ ->
        do
            putStrLn $ "Method not found: " ++ n
            putStrLn $ "In object: " ++ (show o)



findReceiver :: String -> Context -> GraceObject
findReceiver name ctx =
    findReceiver' name $ localScope ctx

findReceiver' :: String -> GraceObject -> GraceObject
findReceiver' name o@(BaseObject outer meths) =
    case Data.Map.lookup name meths of
        Just m -> o
        Nothing -> findReceiver' name outer

findReceiver' name o@(LocalScope outer vars) =
    if isSuffixOf "(0)" name
        then
            case Data.Map.lookup (stripSuffix "(0)" name) vars of
                Just m -> o
                Nothing -> findReceiver' name outer
        else if isSuffixOf ":=(1)" name
            then
                case Data.Map.lookup (stripSuffix ":=(1)" name) vars of
                    Just m -> o
                    Nothing -> findReceiver' name outer
            else if isPrefixOf "def " name
                then
                    case Data.Map.lookup (stripPrefix "def " name) vars of
                        Just m -> o
                        Nothing -> findReceiver' name outer
                else
                    findReceiver' name outer

findReceiver' name o = GraceErrorObject $ "Receiver not found: " ++ name ++ " in " ++ (show o)

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