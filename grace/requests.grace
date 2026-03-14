import "collections" as collections


class part(nm, args, typeargs) {
    def name is public = nm
    def arguments is public = args
    def typeArguments is public = typeargs
}

class request(pts) {
    def name is public = pts.map { x -> x.name ++ "(" ++ x.arguments.size ++ ")" } combine { a, b -> a ++ b }
    def parts is public = pts

    method at(index) {
        if ((index < 1) || (index > parts.size)) then {
            Exception.raise("Index out of bounds: " ++ index)
        }
        return parts.at(index)
    }

    method first {
        if (parts.size == 0) then {
            Exception.raise("Request is empty")
        }
        return parts.first
    }

    method last {
        if (parts.size == 0) then {
            Exception.raise("Request is empty")
        }
        return parts.last
    }

    method asString {
        "request of {name}"
    }

}

method nullary(name) {
    def parts = collections.list []
    parts.add(part(name, collections.list [], collections.list []))
    return request(parts)
}

method unary(name, arg) {
    def parts = collections.list []
    def args = collections.list []
    args.add(arg)
    parts.add(part(name, args, collections.list []))
    return request(parts)
}

method binary(name, arg1, arg2) {
    def parts = collections.list []
    def args = collections.list []
    args.add(arg1)
    args.add(arg2)
    parts.add(part(name, args, collections.list []))
    return request(parts)
}

method twoPart(name1, name2, arg1, arg2) {
    def parts = collections.list []
    def args1 = collections.list []
    def args2 = collections.list []
    args1.add(arg1)
    args2.add(arg2)
    parts.add(part(name1, args1, collections.list []))
    parts.add(part(name2, args2, collections.list []))
    return request(parts)
}
