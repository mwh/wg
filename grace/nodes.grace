import "collections" as collections
import "objects" as objects
import "requests" as requests

class evaluationContext(newScope) {
    def scope is public = newScope

    method withScope(newScope) {
        evaluationContext(newScope)
    }

    method findReceiver(req) {
        scope.findReceiver(req.name)
    }

    method findReturnScope {
        scope.findReturnScope
    }

}

method nil {
    collections.list
}

method one(item) {
    def ret = collections.list
    ret.append(item)
    ret
}

method cons(head, list) {
    list.prepend(head)
    list
}

method strLit(val) {
    object {
        def kind is public = "strLit"
        def value is public = val

        method evaluate(context) {
            objects.graceString(value)
        }

        method evaluateDeclaration(ctx) { }
    }
}

method numLit(val) {
    object {
        def kind is public = "numLit"
        def value is public = val

        method evaluate(context) {
            objects.graceNumber(value)
        }

        method evaluateDeclaration(ctx) { }
    }
}

method part(nm, args) { requests.part(nm, args) }

method lexReq(partList) {
    object {
        def kind is public = "lexReq"
        def parts is public = partList
        def name is public = partList.map { x -> x.name ++ "(" ++ x.arguments.size ++ ")" } combine { a, b -> a ++ b }

        method evaluate(context) {
            def req = requests.request(parts.map { x -> part(x.name, x.arguments.map { arg -> arg.evaluate(context) }) })
            def receiver = context.findReceiver(req)
            receiver.request(req)
        }

        method evaluateDeclaration(ctx) { }
    }
}

method dotReq(rec, partList) {
    object {
        def kind is public = "dotReq"
        def receiver is public = rec
        def parts is public = partList
        def name is public = partList.map { x -> x.name ++ "(" ++ x.arguments.size ++ ")" } combine { a, b -> a ++ b }

        method evaluate(context) {
            def req = requests.request(parts.map { x -> part(x.name, x.arguments.map { arg -> arg.evaluate(context) }) })
            def receiverObj = receiver.evaluate(context)
            receiverObj.request(req)
        }

        method evaluateDeclaration(ctx) { }
    }
}

method assn(lhs, val) {
    object {
        def kind is public = "assn"
        def destination is public = lhs
        def value is public = val

        method evaluate(context) {
            if (destination.kind == "lexReq") then {
                def name = destination.parts.at(1).name ++ ":="
                def evaluatedValue = value.evaluate(context)
                def req = requests.request(one(part(name, one(evaluatedValue))))
                def receiver = context.findReceiver(req)
                receiver.request(req)
            } elseif { destination.kind == "dotReq" } then {
                def receiverObj = destination.receiver.evaluate(context)
                def evaluatedValue = value.evaluate(context)
                def name = destination.parts.at(1).name ++ ":="
                def req = requests.request(one(part(name, one(evaluatedValue))))
                receiverObj.request(req)
            } else {
                Exception.raise("Invalid assignment target: " ++ destination.kind)
            }
        }

        method evaluateDeclaration(ctx) { }
    }
}

method varDec(nm, dtype, anns, val) {
    object {
        def kind is public = "varDec"
        def name is public = nm
        def decType is public = dtype
        def annotations is public = anns
        def value is public = val

        method evaluate(context) {
            if (value.size > 0) then {
                context.scope.request(requests.request(one(part(name ++ ":=", one(value.at(1).evaluate(context))))))
            }
        }

        method evaluateDeclaration(ctx) {
            var storedValue
            ctx.scope.addMethod(name ++ ":=(1)") body { req ->
                storedValue := req.at(1).arguments.at(1)
            }
            ctx.scope.addMethod(name ++ "(0)") body { req ->
                storedValue
            }
        }
    }
}

method defDec(nm, dtype, anns, val) {
    object {
        def kind is public = "defDec"
        def name is public = nm
        def decType is public = dtype
        def annotations is public = anns
        def value is public = val

        method evaluate(context) {
            context.scope.request(requests.unary("def " ++ name, value.evaluate(context)))
        }

        method evaluateDeclaration(ctx) {
            var storedValue
            ctx.scope.addMethod(name ++ "(0)") body { req ->
                storedValue
            }
            ctx.scope.addMethod("def " ++ name ++ "(1)") body { req ->
                storedValue := req.at(1).arguments.at(1)
            }

        }
    }
}

class identifierDeclaration(nm, dtype) {
    def kind is public = "identifierDeclaration"
    def name is public = nm
    def decType is public = dtype

    method evaluate(context) {
        context.scope.request(requests.request(one(part(name ++ "(0)", nil))))
    }

    method evaluateDeclaration(ctx) {
        ctx.scope.addMethod(name ++ "(0)") body { req -> done }
    }
}

class methDec(pts, dtype, anns, bd) {
    def kind is public = "methDec"
    def parts = pts
    def name is public = parts.map { x -> x.name ++ "(" ++ x.arguments.size ++ ")" } combine { a, b -> a ++ b }
    def returnType is public = dtype
    def annotations is public = anns
    def body is public = bd

    method evaluate(context) { }

    method evaluateDeclaration(ctx) {
        def selfBinding = ctx.scope
        ctx.scope.addMethod(name) body { req ->
            def scope = objects.graceObject(selfBinding)
            scope.canReturn := true
            def context = ctx.withScope(scope)
            def paramNames = parts.flatMap { pt -> pt.arguments.map { arg -> arg.name } }
            def args = req.parts.flatMap { pt -> pt.arguments }
            paramNames.zip(args) each { p, a ->
                scope.addMethod(p ++ "(0)") body { req -> a }
            }
            body.each { item ->
                item.evaluateDeclaration(context)
            }
            var lastValue := objects.graceDone
            try {
                body.each { item ->
                    lastValue := item.evaluate(context)
                }
            } catch { e : objects.Return ->
                if (scope.hasReturned) then {
                    lastValue := scope.returnedValue
                } else {
                    e.reraise
                }
            }
            lastValue
        }
    }
}

class importStmt(src, bnd) {
    def kind is public = "importStmt"
    def source is public = src
    def binding is public = bnd

    method evaluate(context) {
        
    }

    method evaluateDeclaration(ctx) {
        if (source == "ast") then {
            ctx.scope.addMethod("ast(0)") body { req ->
                astModule
            }
        } else {
            Exception.raise("Import statements not currently implemented in this evaluator.")
        }
    }
}

class dialectStmt(src) {
    def kind is public = "dialectStmt"
    def source is public = src

    method evaluate(context) {
        
    }

    method evaluateDeclaration(ctx) {
        Exception.raise("Dialect import statements not currently implemented in this evaluator.")
    }
}

class comment(text) {
    def kind is public = "comment"
    def value is public = text

    method evaluate(context) { }

    method evaluateDeclaration(ctx) { }
}

class interpStr(beforeStr, expr, afterNode) {
    def kind is public = "interpStr"
    def before is public = beforeStr
    def expression is public = expr
    def rest is public = afterNode

    method evaluate(context) {
        def evaluatedExpr = expression.evaluate(context)
        def evaluatedAsString = evaluatedExpr.request(requests.request(one(part("asString", nil))))
        def restStr = rest.evaluate(context)
        objects.graceString(before ++ evaluatedExpr.value ++ restStr.value)
    }

    method evaluateDeclaration(ctx) { }
}

class returnStmt(val) {
    def kind is public = "returnStmt"
    def value is public = val

    method evaluate(context) {
        def evaluatedValue = value.evaluate(context)
        def returnScope = context.findReturnScope
        returnScope.hasReturned := true
        returnScope.returnedValue := evaluatedValue
        objects.Return.raise "Return outside of method extent"
    }

    method evaluateDeclaration(ctx) { }
}

class block(params, bd) {
    def kind is public = "block"
    def parameters is public = params
    def body is public = bd

    method evaluate(context) {
        objects.graceBlock { req ->
            def scope = objects.graceObject(context.scope)
            def ctx = context.withScope(scope)

            def args = req.at(1).arguments
            parameters.zip(args) each { p, a ->
                scope.addMethod(p.name ++ "(0)") body { req -> a }
            }

            body.each { item ->
                item.evaluateDeclaration(ctx)
            }

            var lastValue := objects.graceDone
            body.each { item ->
                lastValue := item.evaluate(ctx)
            }
            lastValue
        }
    }

    method evaluateDeclaration(ctx) { }
}

method objCons(bd, anns) {
    object {
        def kind is public = "objCons"
        def body is public = bd
        def annotations is public = anns

        method evaluate(context) {
            def obj = objects.graceObject(context.scope)
            def ctx = context.withScope(obj)

            obj.addMethod("self(0)") body { req ->
                obj
            }

            body.each { item ->
                item.evaluateDeclaration(ctx)
            }

            body.each { item ->
                item.evaluate(ctx)
            }
            obj
        }

        method evaluateDeclaration(ctx) { }
    }
}

class listWrapper(l) {
    def list = l
    
    method size { list.size }
    method firstItem { list.firstItem }
    method lastItem { list.lastItem }
    method append(item) { list.append(item)}
    method prepend(item) { list.prepend(item) }
    method at(index) { list.at(index) }
    method first { list.first }
    method last { list.last }
    method each(block) { list.each(block) }
    method map(transform) { listWrapper(list.map(transform)) }
    method flatMap(transform) { listWrapper(list.flatMap(transform)) }
    method map(transform) combine(combiner) {
        list.map(transform) combine(combiner)
    }
    method zip(otherList) each(block) {
        list.zip(otherList) each(block)
    }

    method request(req) {
        match (req.name)
            case { "map(1)" -> 
                def blk = req.at(1).arguments.at(1)
                return listWrapper(list.map { x ->
                    def applyReq = requests.unary("apply", x)
                    blk.request(applyReq)
                    }
                    )
            }
            case { "reversed(1)" ->
                // Only nil is ever passed by user code, ignore argument
                return listWrapper(list.reversed)
            }
            case { "asString(0)" ->
                if (list.size == 0) then {
                    return objects.graceString("nil")
                }
                if (list.size == 1) then {
                    return objects.graceString("one(" ++ list.first.request(requests.nullary "asString").value ++ ")")
                }
                return "cons(" ++ list.first.request(requests.nullary "asString").value ++ ", " ++ list.tail.request(requests.nullary "asString").value ++ ")"
            }
    }

    method asString {
        "listWrapper of {list}"
    }
}

def astModule = object {
    method request(req) {
        match (req.name)
            case { "cons(2)" -> 
                req.at(1).arguments.at(2).prepend(req.at(1).arguments.at(1))
                return req.at(1).arguments.at(2)
            }
            case { "nil(0)" -> return listWrapper(nil) }
            case { "one(1)" -> return listWrapper(one(req.at(1).arguments.at(1))) }
            case { "numberNode(1)" -> return numLit(req.at(1).arguments.at(1).value) }
            case { "stringNode(1)" -> return strLit(req.at(1).arguments.at(1).value) }
            case { "interpString(3)" -> return interpStr(req.at(1).arguments.at(1).value, req.at(1).arguments.at(2), req.at(1).arguments.at(3)) }
            case { "block(2)" -> return block(req.at(1).arguments.at(1), req.at(1).arguments.at(2)) }
            case { "defDecl(4)" -> return defDec(req.at(1).arguments.at(1).value, req.at(1).arguments.at(2), req.at(1).arguments.at(3), req.at(1).arguments.at(4)) }
            case { "varDecl(4)" -> return varDec(req.at(1).arguments.at(1).value, req.at(1).arguments.at(2), req.at(1).arguments.at(3), req.at(1).arguments.at(4)) }
            case { "lexicalRequest(2)" -> return lexReq(req.at(1).arguments.at(2)) }
            case { "explicitRequest(2)" -> return dotReq(req.at(1).arguments.at(1), req.at(1).arguments.at(2)) }
            case { "explicitRequest(3)" -> return dotReq(req.at(1).arguments.at(2), req.at(1).arguments.at(3)) }
            case { "part(2)" -> return part(req.at(1).arguments.at(1).value, req.at(1).arguments.at(2)) }
            case { "methodDecl(4)" -> return methDec(req.at(1).arguments.at(1), req.at(1).arguments.at(2), req.at(1).arguments.at(3), req.at(1).arguments.at(4)) }
            case { "objectConstructor(2)" -> return objCons(req.at(1).arguments.at(1), req.at(1).arguments.at(2)) }
            case { "assign(2)" -> return assn(req.at(1).arguments.at(1), req.at(1).arguments.at(2)) }
            case { "returnStmt(1)" -> return returnStmt(req.at(1).arguments.at(1)) }
            case { "identifierDeclaration(2)" -> return identifierDeclaration(req.at(1).arguments.at(1).value, req.at(1).arguments.at(2)) }
            case { "comment(1)" -> return comment(req.at(1).arguments.at(1).value) }
            case { "importStmt(2)" -> return importStmt(req.at(1).arguments.at(1).value, req.at(1).arguments.at(2)) }
            case { "dialectStmt(1)" -> return dialectStmt(req.at(1).arguments.at(1).value) }
            case { other -> Exception.raise "Unknown request on mocked ast module: {other}" }
    }
}
