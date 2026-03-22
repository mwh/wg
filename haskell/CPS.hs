module CPS where

import Ast
import Data.Map (insert, empty, Map, fromList)
import qualified Data.Map as Data.Map
import Data.IORef
import Data.Char
import Data.List (isSuffixOf, isPrefixOf)
import Runtime


toFunc :: ASTNode -> (Context -> IO ())
toFunc (NumberNode v) =
    \ctx ->
        do
            (continuation ctx) $ GraceNumber v

toFunc (ExplicitRequest receiverNode parts) =
    let name = partsToName parts
        receiverFunc = toFunc receiverNode
        argSeq = do
            Part _ a _ <- parts
            argNode <- a
            return $ toFunc argNode
    in
        \ctx ->
            receiverFunc $ withCont ctx (\receiver ->
                let method = getMethod name receiver
                in
                    do
                        evalArgSeq ctx argSeq (\args ->
                            do
                                method ctx args
                            )
            )
toFunc (ObjectConstructor body anns) =
    \ctx -> evalObjectConstructor body ctx Nothing

toFunc (VarDecl name _ _ init) =
    \ctx ->
        do
            case init of
                [] -> continuation ctx $ GraceDone
                [initialiser] ->
                    let initFunc = toFunc initialiser
                        slf = localScope ctx
                        setter = getMethod (name ++ ":=(1)") slf
                    in
                        initFunc $ withCont ctx (\val ->
                            do
                                setter ctx [val]
                            )

toFunc (DefDecl name _ _ initialiser) =
    \ctx ->
        do
            let initFunc = toFunc initialiser
                slf = localScope ctx
                setter = getMethod ("def " ++ name) slf
            initFunc $ withCont ctx (\val ->
                do
                    setter ctx [val]
                )

toFunc (Comment _) =
    \ctx ->
        do
            (continuation ctx) GraceDone

toFunc (StringNode s) =
    \ctx ->
        do
            (continuation ctx) $ GraceString s

toFunc (Assign lhs rhs) =
    case lhs of
        LexicalRequest parts ->
            let Part name _ _ = parts !! 0
                rhsFunc = toFunc rhs
            in \ctx ->
                    let slf = localScope ctx
                        receiver = findReceiver (name ++ ":=(1)") ctx
                        setter = getMethod (name ++ ":=(1)") receiver
                    in
                        rhsFunc $ withCont ctx (\val ->
                            do
                                setter ctx [val]
                            )
        ExplicitRequest receiverNode parts ->
            let Part name _ _ = parts !! 0
                receiverFunc = toFunc receiverNode
                rhsFunc = toFunc rhs
            in
                \ctx ->
                    receiverFunc $ withCont ctx (\receiver ->
                        let setter = getMethod (name ++ ":=(1)") receiver
                        in
                            do
                                rhsFunc $ withCont ctx (\val ->
                                    do
                                        setter ctx [val]
                                    )
                        )

toFunc (LexicalRequest parts) =
    let name = partsToName parts
        argSeq = do
            Part _ a _ <- parts
            argNode <- a
            return $ toFunc argNode
    in
        \ctx ->
            let receiver = findReceiver name ctx
            in
                let method = getMethod name receiver
                in
                    do
                        evalArgSeq ctx argSeq (\args ->
                            do
                                method ctx args
                            )

toFunc (MethodDecl parts _ _ body) =
    \ctx -> continuation ctx $ GraceDone

toFunc (ReturnStmt expr) =
    let func = toFunc expr
    in
        \ctx ->
            func $ withCont ctx (\val ->
                (returnCont ctx) val
            )

toFunc (Block params body) =
    let bodyFuncs = map toFunc body
    in
        \ctx ->
            let folded =
                    if length bodyFuncs > 0
                        then Prelude.foldr1
                            (\f acc ctx' -> f (withCont ctx' (\_ -> acc ctx')))
                            bodyFuncs
                        else (\ctx' -> (continuation ctx') GraceDone)
            in
                do
                    continuation ctx $ GraceBlock [name | (IdentifierDeclaration name _) <- params] folded (varNames body) ctx

toFunc (ImportStmt name binding) =
    \ctx ->
        do
            let scope = localScope ctx
                mods = importModules ctx
                mod = Data.Map.lookup name mods
            case mod of
                Just modObj ->
                    do
                        let setter = getMethod ("import " ++ name) scope
                        setter ctx [modObj]
                Nothing ->
                    putStrLn $ "Cannot import " ++ name ++ ": external imports not supported in this version of Grace (use -i option of Java version to inline)"

toFunc (DialectStmt name) =
    \ctx ->
        do
            putStrLn $ "Cannot use dialect " ++ name ++ ": external imports not supported in this version of Grace"

toFunc (InterpString before expr next) =
    let exprFunc = toFunc expr
        nextFunc = toFunc next
    in
        \ctx ->
            exprFunc $ withCont ctx (\val ->
                let asString = getMethod "asString(0)" val
                in
                    asString (withCont ctx (\val ->
                        nextFunc $ withCont ctx (\nextVal ->
                            continuation ctx $ GraceString (before ++ (show val) ++ (show nextVal))
                        )
                    ) ) []
            )

toFunc (InterfaceConstructor meths) =
    \ctx -> continuation ctx $ GraceDone

toFunc (InheritStmt _) =
    \ctx -> continuation ctx $ GraceDone

toFunc (UseStmt _) =
    \ctx -> continuation ctx $ GraceDone

toFunc (Lineup elems) =
    let elemFuncs = map toFunc elems
    in
        \ctx ->
            evalElemList ctx elemFuncs [] $ \vals ->
                continuation ctx $ Runtime.makeLineup vals

toFunc (TypeDecl name init) =
    \ctx ->
        do
            let slf = localScope ctx
                setter = getMethod ("type " ++ name) slf
                initFunc = toFunc init
            initFunc $ withCont ctx (\val ->
                    do
                        setter ctx [val]
                    )

evalObjectConstructor :: [ASTNode] -> Context -> Maybe (IORef GraceObject, Map String (Context -> [GraceObject] -> IO ())) -> IO ()
evalObjectConstructor body ctx inheritingAs =
    do
        -- If inheriting, reuse the self ref and start with existing methods
        (self, existingMeths) <- case inheritingAs of
            Just (selfRef, meths) -> return (selfRef, meths)
            Nothing -> do
                selfRef <- newIORef GraceDone
                return (selfRef, Data.Map.empty)
        newMeths <- makeMethods body self
        -- Merge: existing methods take precedence (child overrides parent)
        let mergedMeths = Data.Map.union existingMeths newMeths
        let methsSelf = case inheritingAs of
                Nothing -> insert "self(0)" (\ctx' _ -> do
                        selfObj <- readIORef self
                        continuation ctx' $ selfObj) mergedMeths
                Just _ -> mergedMeths  -- self(0) already in existingMeths
        let obj = BaseObject (localScope ctx) methsSelf
        writeIORef self obj
        let selfCtx = withSelf ctx obj
        -- Filter body: skip MethodDecl (already handled), skip InheritStmt (handled below)
        let stmtFuncs = [toFunc s | s <- body, not (isMethodDecl s), not (isInheritStmt s), not (isUseStmt s)]
        -- Find inherit statement if present
        let inheritStmts = [expr | InheritStmt expr <- body]
        -- Find use statements
        let useStmts = [expr | UseStmt expr <- body]
        -- Process use statements: evaluate the expression, copy methods
        let processUses ctx' useCont =
                case useStmts of
                    [] -> useCont ctx'
                    _ -> processUseList useStmts self ctx' useCont
        -- Process inherit: call parent method with inherit(1) passing self
        let processInherit ctx' afterInherit =
                case inheritStmts of
                    [] -> afterInherit ctx'
                    [expr] -> callInheritMethod expr self ctx' afterInherit
                    _ -> do putStrLn "Multiple inherit statements not allowed"
                            return ()
        -- Evaluation order: use -> add fields/methods -> inherit -> run body statements
        processUses selfCtx $ \ctx1 -> do
            -- Re-read self since use may have updated it
            selfObj <- readIORef self
            let ctx2 = withSelf ctx1 selfObj
            processInherit ctx2 $ \ctx3 -> do
                -- Re-read self since inherit may have updated it
                selfObj' <- readIORef self
                let ctx4 = withSelf ctx3 selfObj'
                let folded = Prelude.foldr
                        (\f acc c -> f (withCont c (\_ -> acc c)))
                        (\c -> (continuation ctx) selfObj')
                        stmtFuncs
                folded ctx4

isMethodDecl :: ASTNode -> Bool
isMethodDecl (MethodDecl _ _ _ _) = True
isMethodDecl _ = False

isInheritStmt :: ASTNode -> Bool
isInheritStmt (InheritStmt _) = True
isInheritStmt _ = False

isUseStmt :: ASTNode -> Bool
isUseStmt (UseStmt _) = True
isUseStmt _ = False

callInheritMethod :: ASTNode -> IORef GraceObject -> Context -> (Context -> IO ()) -> IO ()
callInheritMethod expr selfRef ctx afterInherit =
    case expr of
        LexicalRequest parts ->
            let name = partsToName parts
                inheritName = name ++ "inherit(1)"
                argSeq = do
                    Part _ a _ <- parts
                    argNode <- a
                    return $ toFunc argNode
            in do
                selfObj <- readIORef selfRef
                let receiver = findReceiver name ctx
                let method = getMethod inheritName receiver
                evalArgSeq ctx argSeq $ \args -> do
                    method (withCont ctx (\resultObj -> do
                        writeIORef selfRef resultObj
                        afterInherit ctx)) (args ++ [selfObj])
        ExplicitRequest receiverNode parts ->
            let name = partsToName parts
                inheritName = name ++ "inherit(1)"
                receiverFunc = toFunc receiverNode
                argSeq = do
                    Part _ a _ <- parts
                    argNode <- a
                    return $ toFunc argNode
            in
                receiverFunc $ withCont ctx $ \receiver -> do
                    selfObj <- readIORef selfRef
                    evalArgSeq ctx argSeq $ \args -> do
                        let method = getMethod inheritName receiver
                        method (withCont ctx (\resultObj -> do
                            writeIORef selfRef resultObj
                            afterInherit ctx)) (args ++ [selfObj])
        _ -> do
            putStrLn $ "Invalid inherit expression: " ++ show expr
            return ()

processUseList :: [ASTNode] -> IORef GraceObject -> Context -> (Context -> IO ()) -> IO ()
processUseList [] _ ctx cont = cont ctx
processUseList (expr:rest) selfRef ctx cont =
    let exprFunc = toFunc expr
    in exprFunc $ withCont ctx $ \mixinObj -> do
        case mixinObj of
            BaseObject _ mixinMeths -> do
                selfObj <- readIORef selfRef
                case selfObj of
                    BaseObject parent existMeths -> do
                        -- Copy methods from mixin that don't already exist, skip builtins
                        let skipNames = ["==(1)", "!=(1)", "asString(0)", "asDebugString(0)", "hash(0)", "::(1)", "self(0)"]
                        let filtered = Data.Map.filterWithKey (\k _ -> k `notElem` skipNames) mixinMeths
                        let merged = Data.Map.union existMeths filtered
                        let newObj = BaseObject parent merged
                        writeIORef selfRef newObj
                        let ctx' = withSelf ctx newObj
                        processUseList rest selfRef ctx' cont
                    _ -> do
                        putStrLn "Use: self is not a BaseObject"
                        cont ctx
            _ -> do
                putStrLn "Use: mixin is not a BaseObject"
                cont ctx

varNames :: [ASTNode] -> [String]
varNames [] = []
varNames (h:t) =
    case h of
        VarDecl name _ _ _ -> name : varNames t
        DefDecl name _ _ _ -> name : varNames t
        ImportStmt name _ -> name : varNames t
        TypeDecl name _ -> name : varNames t
        _ -> varNames t

makeMethods :: [ASTNode] -> IORef GraceObject -> IO (Map String (Context -> [GraceObject] -> IO ()))
makeMethods [] self = do
    return empty
makeMethods (stmt:rest) self =
    do
        cell <- newIORef GraceDone
        case stmt of
            VarDecl name _ _ init ->
                do
                    let setterName = name ++ ":=(1)"
                        getterName = name ++ "(0)"
                        getter = \ctx args ->
                            do
                                val <- readIORef cell
                                (continuation ctx) $ val
                        setter = \ctx [val] ->
                            do
                                writeIORef cell val
                                (continuation ctx) $ GraceDone
                    restMeths <- makeMethods rest self
                    return $ insert getterName getter $ insert setterName setter restMeths
            DefDecl name _ _ init ->
                do
                    let setterName = "def " ++ name
                        getterName = name ++ "(0)"
                        getter = \ctx args ->
                            do
                                val <- readIORef cell
                                (continuation ctx) $ val
                        setter = \ctx [val] ->
                            do
                                writeIORef cell val
                                (continuation ctx) $ GraceDone
                    restMeths <- makeMethods rest self
                    return $ insert getterName getter $ insert setterName setter restMeths
            TypeDecl name init ->
                do
                    let setterName = "type " ++ name
                        getterName = name ++ "(0)"
                        getter = \ctx args ->
                            do
                                val <- readIORef cell
                                (continuation ctx) $ val
                        setter = \ctx [val] ->
                            do
                                writeIORef cell val
                                (continuation ctx) $ GraceDone
                    restMeths <- makeMethods rest self
                    return $ insert getterName getter $ insert setterName setter restMeths
            ImportStmt _ (IdentifierDeclaration name _) ->
                do
                    let setterName = "import " ++ name
                        getterName = name ++ "(0)"
                        getter = \ctx args ->
                            do
                                val <- readIORef cell
                                (continuation ctx) $ val
                        setter = \ctx [val] ->
                            do
                                writeIORef cell val
                                (continuation ctx) $ GraceDone
                    restMeths <- makeMethods rest self
                    return $ insert getterName getter $ insert setterName setter restMeths
            MethodDecl parts _ _ body ->
                let bodyFuncs = map toFunc body
                    name = partsToName parts
                in
                    do
                        restMeths <- makeMethods rest self
                        let method =
                                \ctx args ->
                                    do
                                        let selfObj = Runtime.self ctx
                                        let params = parts >>= (\(Part _ a _) -> a)
                                        let params' = map (\(IdentifierDeclaration name _) -> name) params
                                        scopeMap <- makeScopeMap params' args (varNames body)
                                        let
                                            selfCtx = withSelf ctx selfObj
                                            selfScope = LocalScope selfObj scopeMap
                                            selfCtx' = withScope selfCtx selfScope
                                            retCont = withReturn selfCtx' $ continuation ctx
                                            folded = if length bodyFuncs > 0
                                                    then Prelude.foldr1
                                                        (\f acc ctx' -> f (withCont ctx' (\_ -> acc ctx')))
                                                        bodyFuncs
                                                    else (\ctx' -> (continuation retCont) GraceDone)
                                        folded retCont
                        -- Check if body ends with ObjectConstructor; if so, generate inherit variant
                        let withInherit = case lastMay body of
                                Just (ObjectConstructor ocBody ocAnns) ->
                                    let inheritName = name ++ "inherit(1)"
                                        inheritMethod = \ctx args ->
                                            do
                                                selfObj <- readIORef self
                                                let allArgs = args
                                                    inheritObj = last allArgs
                                                    normalArgs = init allArgs
                                                    params = parts >>= (\(Part _ a _) -> a)
                                                    params' = map (\(IdentifierDeclaration name _) -> name) params
                                                -- Execute all body statements except the last (OC)
                                                scopeMap <- makeScopeMap params' normalArgs (varNames body)
                                                let selfCtx = withSelf ctx selfObj
                                                    selfScope = LocalScope selfObj scopeMap
                                                    selfCtx' = withScope selfCtx selfScope
                                                    retCont = withReturn selfCtx' $ continuation ctx
                                                    preBodyFuncs = map toFunc (init body)
                                                case inheritObj of
                                                    BaseObject _ inheritMeths -> do
                                                        inheritSelfRef <- newIORef inheritObj
                                                        let runPreBody ctx'' afterPre =
                                                                if null preBodyFuncs
                                                                    then afterPre ctx''
                                                                    else let folded = Prelude.foldr
                                                                                (\f acc c -> f (withCont c (\_ -> acc c)))
                                                                                afterPre
                                                                                preBodyFuncs
                                                                         in folded ctx''
                                                        runPreBody retCont $ \ctx'' -> do
                                                            -- Evaluate the tail ObjectConstructor with inherit
                                                            evalObjectConstructor ocBody ctx'' (Just (inheritSelfRef, inheritMeths))
                                                    _ -> do
                                                        putStrLn "Inherit: argument is not a BaseObject"
                                                        return ()
                                    in insert inheritName inheritMethod
                                _ -> id
                        return $ withInherit $ insert name method restMeths
            _ -> makeMethods rest self

lastMay :: [a] -> Maybe a
lastMay [] = Nothing
lastMay xs = Just (last xs)

evalElemList :: Context -> [Context -> IO ()] -> [GraceObject] -> ([GraceObject] -> IO ()) -> IO ()
evalElemList ctx [] acc cont = cont (reverse acc)
evalElemList ctx (f:fs) acc cont =
    f $ withCont ctx $ \val ->
        evalElemList ctx fs (val:acc) cont

