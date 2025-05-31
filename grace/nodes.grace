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
            context.scope.request(requests.request(one(part("def " ++ name, one(value.evaluate(context))))))
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
            def context = ctx.withScope(scope)
            def paramNames = parts.flatMap { pt -> pt.arguments.map { arg -> arg.name } }
            def args = req.parts.flatMap { pt -> pt.arguments }
            paramNames.zip(args) each { p, a ->
                scope.addMethod(p ++ "(0)") body { req -> a }
            }
            var lastValue := objects.graceDone
            body.each { item ->
                lastValue := item.evaluate(context)
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
        Exception.raise("Import statements not currently implemented in this evaluator.")
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
        print "Return statements currently non-functional in this evaluator; consider tail return if possible."
        objects.Return.raise(evaluatedValue)
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
