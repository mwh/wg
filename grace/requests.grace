
class part(nm, args) {
    def name is public = nm
    def arguments is public = args
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

    method asString {
        "request of {name}"
    }

}