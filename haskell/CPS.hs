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
            Part _ a <- parts
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
    \ctx ->
        do
            self <- newIORef GraceDone
            meths <- makeMethods body self
            let methsSelf = insert "self(0)" (\ctx _ -> do
                    selfObj <- readIORef self
                    continuation ctx $ selfObj) meths
            let obj = BaseObject (localScope ctx) methsSelf
            writeIORef self obj
            let selfCtx = withSelf ctx obj
            let bodyFuncs = map toFunc body
            let retCont = withCont selfCtx (\_ -> (continuation ctx) obj)

            let folded = Prelude.foldr
                    (\f acc ctx' -> f (withCont ctx' (\_ -> acc ctx')))
                    (\ctx' -> (continuation ctx') obj)
                    bodyFuncs
            folded selfCtx


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
            let Part name _ = parts !! 0
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
            let Part name _ = parts !! 0
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
            Part _ a <- parts
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
                    continuation ctx $ GraceBlock [name | (LexicalRequest [Part name _]) <- params] folded (varNames body) ctx

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

varNames :: [ASTNode] -> [String]
varNames [] = []
varNames (h:t) =
    case h of
        VarDecl name _ _ _ -> name : varNames t
        DefDecl name _ _ _ -> name : varNames t
        ImportStmt name _ -> name : varNames t
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
                        fakeSelf <- newIORef GraceDone
                        let method =
                                \ctx args ->
                                    do
                                        selfObj <- readIORef self
                                        let params = parts >>= (\(Part _ a) -> a)
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
                        return $ insert name method restMeths
            _ -> makeMethods rest self
