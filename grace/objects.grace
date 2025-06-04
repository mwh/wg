import "collections" as collections
import "requests" as requests

def Return = Exception.refine "Return"
def GraceException = Exception.refine "Grace-Exception"

def terminalObject = object {

    method request(req) {
        match (req.name)
            case { "print(1)" ->
                def arg = req.at(1).arguments.at(1)
                def str = arg.request(requests.nullary "asString")
                print("Grace-in-Grace print: " ++ str.value)
                return graceDone
            }
            case { "if(1)then(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)
                condition.request(requests.unary("ifTrue", thenBlock))
                return graceDone
            }
            case { "if(1)then(1)else(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)
                def elseBlock = req.at(3).arguments.at(1)
                return condition.request(requests.twoPart("ifTrue", "ifFalse", thenBlock, elseBlock))
            }
            case { "while(1)do(1)" ->
                def condition = req.at(1).arguments.at(1)
                def bodyBlock = req.at(2).arguments.at(1)
                def applyReq = requests.nullary "apply"
                while { condition.request(applyReq).booleanValue } do {
                    bodyBlock.request(applyReq)
                }
                return graceDone
            }
            case { "true(0)" ->
                return graceBoolean(true)
            }
            case { "false(0)" ->
                return graceBoolean(false)
            }
            case { "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)
                def elseBlock = req.last.arguments.at(1)

                def applyReq = requests.nullary "apply"
                if (condition.booleanValue) then {
                    return thenBlock.request(applyReq)
                } elseif { req.parts.at(3).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(4).arguments.at(1).request(applyReq)
                } elseif { req.parts.at(5).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(6).arguments.at(1).request(applyReq)
                } elseif { req.parts.at(7).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(8).arguments.at(1).request(applyReq)
                } else {
                    return elseBlock.request(applyReq)
                }
            }
            case { "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)
                def applyReq = requests.nullary "apply"
                if (condition.booleanValue) then {
                    return thenBlock.request(applyReq)
                } elseif { req.parts.at(3).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(4).arguments.at(1).request(applyReq)
                } elseif { req.parts.at(5).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(6).arguments.at(1).request(applyReq)
                } elseif { req.parts.at(7).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(8).arguments.at(1).request(applyReq)
                }
                return graceDone
            }
            case { "if(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)
                def elseBlock = req.last.arguments.at(1)

                def applyReq = requests.nullary "apply"
                if (condition.booleanValue) then {
                    return thenBlock.request(applyReq)
                } elseif { req.parts.at(3).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(4).arguments.at(1).request(applyReq)
                } elseif { req.parts.at(5).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(6).arguments.at(1).request(applyReq)
                } else {
                    return elseBlock.request(applyReq)
                }
            }
            case { "if(1)then(1)elseif(1)then(1)elseif(1)then(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)

                def applyReq = requests.nullary "apply"
                if (condition.booleanValue) then {
                    return thenBlock.request(applyReq)
                } elseif { req.parts.at(3).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(4).arguments.at(1).request(applyReq)
                } elseif { req.parts.at(5).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(6).arguments.at(1).request(applyReq)
                }
                return graceDone
            }
            case { "if(1)then(1)elseif(1)then(1)else(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)
                def elseBlock = req.last.arguments.at(1)

                def applyReq = requests.nullary "apply"
                if (condition.booleanValue) then {
                    return thenBlock.request(applyReq)
                } elseif { req.parts.at(3).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(4).arguments.at(1).request(applyReq)
                } else {
                    return elseBlock.request(applyReq)
                }
            }
            case { "if(1)then(1)elseif(1)then(1)" ->
                def condition = req.at(1).arguments.at(1)
                def thenBlock = req.at(2).arguments.at(1)

                def applyReq = requests.nullary "apply"
                if (condition.booleanValue) then {
                    return thenBlock.request(applyReq)
                } elseif { req.parts.at(3).arguments.at(1).request(applyReq).booleanValue } then {
                    return req.parts.at(4).arguments.at(1).request(applyReq)
                }
                return graceDone
            }
            case { "getFileContents(1)" ->
                def fileName = req.at(1).arguments.at(1)
                def contents = getFileContents(fileName.value)
                return graceString(contents)
            }
            case { "Exception(0)" ->
                return graceExceptionKind(GraceException)
            }
            case { other ->
                Exception.raise("Method not found in scope: {other}")
            }
    }

    method hasMethod(name) {
        true
    }

    method findReceiver(name) {
        return self
        if (name == "print(1)") then {
            return self
        }
        Exception.raise("Method not found: " ++ name)
    }
}

def graceDone = object {
    def isDone is public = true

    method request(req) {
        Exception.raise("Method requested on done: " ++ req.name)
    }

    method hasMethod(name) {
        false
    }

    method findReceiver(name) {
        Exception.raise("Method not found in done: " ++ name)
    }
}

class graceObject(scp) {
    def methods = collections.dictionary
    def surroundingScope is public = scp

    var canReturn is public := false
    var returnedValue is public
    var hasReturned is public := false

    method request(req) {
        def name = req.name
        if (!methods.has(name)) then {
            Exception.raise("Method not found in object {self}: " ++ name)
        }
        def meth = methods.at(name)
        meth.apply(req)
    }

    method hasMethod(name) {
        return methods.has(name)
    }

    method findReceiver(name) {
        if (methods.has(name)) then {
            return self
        }
        return surroundingScope.findReceiver(name)
    }

    method addMethod(name) body(body) {
        methods.at(name) put(body)
    }

    method findReturnScope {
        if (canReturn) then {
            return self
        }
        return surroundingScope.findReturnScope
    }

    method asString {
        def meths = methods.keys.map { x -> x } combine { a, b -> a ++ ", " ++ b }
        "graceObject(with methods {meths})"
    }
}

class graceString(val) {
    def value is public = val
    def isString = true

    method request(req) {
        match (req.name)
            case { "asString(0)" ->
                return self
            }
            case { "size(0)" ->
                return graceNumber(value.size)
            }
            case { "==(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value == other.value)
            }
            case { "!=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value != other.value)
            }
            case { "++(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceString(value ++ other.request(requests.nullary "asString").value)
            }
            case { "at(1)" ->
                def index = req.at(1).arguments.at(1)
                return graceString(value.at(index.value))
            }
            case { "substringFrom(1)to(1)" ->
                def startIndex = req.at(1).arguments.at(1)
                def endIndex = req.at(2).arguments.at(1)
                return graceString(value.substring(startIndex.value, endIndex.value))
            }
            case { "replace(1)with(1)" ->
                def oldValue = req.at(1).arguments.at(1)
                def newValue = req.at(2).arguments.at(1)
                return graceString(value.replace(oldValue.value)with(newValue.value))
            }
            case { "firstCodepoint(0)" | "firstCP(0)" ->
                if (value.size == 0) then {
                    Exception.raise("String is empty, no first codepoint")
                }
                return graceNumber(value.at(1).firstCodepoint)
            }
            case { "<(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value < other.value)
            }
            case { ">(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value > other.value)
            }
            case { "<=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value <= other.value)
            }
            case { ">=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value >= other.value)
            }
            case { other ->
                Exception.raise "No such method in string: {other}"
            }
    }

    method asString {
        return "graceString(\"{value}\")"
    }
}

class graceNumber(val) {
    def value is public = val

    method request(req) {
        match (req.name)
            case { "asString(0)" ->
                return graceString(value.asString)
            }
            case { "==(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value == other.value)
            }
            case { "!=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value != other.value)
            }
            case { "+(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceNumber(value + other.value)
            }
            case { "-(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceNumber(value - other.value)
            }
            case { "*(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceNumber(value * other.value)
            }
            case { "/(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceNumber(value / other.value)
            }
            case { ">(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value > other.value)
            }
            case { "<(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value < other.value)
            }
            case { ">=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value >= other.value)
            }
            case { "<=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(value <= other.value)
            }
            case { other ->
                Exception.raise "Method not found in number: {other}"
            }
    }
}

class graceBoolean(val) {
    def booleanValue is public = val

    method request(req) {
        match(req.name)
            case { "asString(0)" ->
                return graceString(booleanValue.asString)
            }
            case { "==(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(booleanValue == other.booleanValue)
            }
            case { "!=(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(booleanValue != other.booleanValue)
            }
            case { "ifTrue(1)" ->
                def thenBlock = req.at(1).arguments.at(1)
                if (booleanValue) then {
                    return thenBlock.request(requests.nullary "apply")
                }
                return graceDone
            }
            case { "ifFalse(1)" ->
                def elseBlock = req.at(1).arguments.at(1)
                if (!booleanValue) then {
                    return elseBlock.request(requests.nullary "apply")
                }
                return graceDone
            }
            case { "ifTrue(1)ifFalse(1)" ->
                def thenBlock = req.at(1).arguments.at(1)
                def falseBlock = req.at(2).arguments.at(1)
                if (booleanValue) then {
                    return thenBlock.request(requests.nullary "apply")
                } else {
                    return falseBlock.request(requests.nullary "apply")
                }
            }
            case { "&&(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(booleanValue && other.booleanValue)
            }
            case { "||(1)" ->
                def other = req.at(1).arguments.at(1)
                return graceBoolean(booleanValue || other.booleanValue)
            }
            case { "not(0)" | "prefix!(0)" ->
                return graceBoolean(!booleanValue)
            }
            case { other ->
                Exception.raise("Method not found in boolean: {other}")
            }
    }
}

class graceBlock(blk) {
    def innerBlock is public = blk

    method request(req) {
        if (req.parts.at(1).name == "apply") then {
            return innerBlock.apply(req)
        }
        Exception.raise("No such method in block: " ++ req.name)
    }

    method hasMethod(name) {
        if (name == "apply(1)") then {
            return true
        }
        false
    }

    method findReceiver(name) {
        if (hasMethod(name)) then {
            return self
        }
        Exception.raise("Method not found in block: " ++ name)
    }
}

class graceExceptionKind(kind) {
    def exceptionKind is public = kind

    method request(req) {
        match (req.name)
            case { "asString(0)" ->
                return graceString("Exception of kind: " ++ exceptionKind.name)
            }
            case { "raise(1)" ->
                def message = req.at(1).arguments.at(1).value
                exceptionKind.raise(message)
            }
            case { "refine(1)" ->
                def newName = req.at(1).arguments.at(1)
                return graceExceptionKind(exceptionKind.refine("Grace-" ++ newName.value))
            }
            case { other ->
                Exception.raise("Method not found in exception kind: {other}")
            }
    }

    method asString {
        return "graceExceptionKind(\"{exceptionKind.name}\")"
    }
}
