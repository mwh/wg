import "collections" as collections
import "requests" as requests

def Return = Exception.refine "Return"

def terminalObject = object {

    method request(req) {
        if (req.name == "print(1)") then {
            def arg = req.at(1).arguments.at(1)
            def parts = collections.list
            parts.append(requests.part("asString", collections.list))
            def rq = requests.request(parts)
            def str = arg.request(rq)
            print(str.value)
            return arg
        }
        Exception.raise("Method not found: " ++ req.name)
    }

    method hasMethod(name) {
        if (name == "print(1)") then {
            return true
        }
        false
    }

    method findReceiver(name) {
        if (name == "print(1)") then {
            return self
        }
        Exception.raise("Method not found: " ++ name)
    }
}

def graceDone = object {
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
            Exception.raise("Method not found: " ++ name)
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
}

class graceString(val) {
    def value is public = val

    method request(req) {
        if (req.name == "asString(0)") then {
            return self
        }
        if (req.name == "size(0)") then {
            return graceNumber(value.size)
        }
        if (req.name == "==(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value == other.value)
        }
        if (req.name == "!=(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value != other.value)
        }
        if (req.name == "++(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceString(value ++ other.value)
        }
        Exception.raise("Method not found: " ++ req.name)
    }
}

class graceNumber(val) {
    def value is public = val

    method request(req) {
        if (req.name == "asString(0)") then {
            return graceString(value.asString)
        }
        if (req.name == "==(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value == other.value)
        }
        if (req.name == "!=(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value != other.value)
        }
        if (req.name == "+(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceNumber(value + other.value)
        }
        if (req.name == "-(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceNumber(value - other.value)
        }
        if (req.name == "*(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceNumber(value * other.value)
        }
        if (req.name == "/(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceNumber(value / other.value)
        }
        if (req.name == ">(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value > other.value)
        }
        if (req.name == "<(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value < other.value)
        }
        if (req.name == ">=(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value >= other.value)
        }
        if (req.name == "<=(1)") then {
            def other = req.at(1).arguments.at(1)
            return graceBoolean(value <= other.value)
        }
        Exception.raise("Method not found: " ++ req.name)
    }
}

class graceBlock(blk) {
    def innerBlock is public = blk

    method request(req) {
        if (req.parts.at(1).name == "apply") then {
            return innerBlock.apply(req)
        }
        Exception.raise("Method not found: " ++ req.name)
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
