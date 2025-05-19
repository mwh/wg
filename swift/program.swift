class Node {

}

class ObjectConstructor : Node {
    var body : [Node]
    var annotations : [Node]

    init(_ body : [Node], _ annotations : [Node]) {
        self.body = body
        self.annotations = annotations
    }
}

class DefDeclaration : Node {
    var name : String
    var type : Node?
    var annotations : [Node]
    var value : Node

    init(_ name : String, _ type : Node?, _ annotations : [Node], _ value : Node) {
        self.name = name
        self.type = type
        self.annotations = annotations
        self.value = value
    }
}

class VarDeclaration : Node {
    var name : String
    var type : Node?
    var annotations : [Node]
    var value : Node?

    init(_ name : String, _ type : Node?, _ annotations : [Node], _ value : Node?) {
        self.name = name
        self.type = type
        self.annotations = annotations
        self.value = value
    }
}

class NumberLiteral : Node {
    var value : Int

    init(_ value : Int) {
        self.value = value
    }
}

class StringLiteral : Node {
    var value : String

    init(_ value : String) {
        self.value = value
    }
}

class InterpolatedString : Node {
    var before : String
    var expression : Node
    var after : Node

    init(_ before : String, _ expression : Node, _ after : Node) {
        self.before = before
        self.expression = expression
        self.after = after
    }
}

class LexicalRequest : Node {
    var parts : [Part]

    init(_ parts : [Part]) {
        self.parts = parts
    }
}

class IdentifierDeclaration : Node {
    var name : String
    var type : Node?

    init(_ name : String, _ type : [Node]?) {
        self.name = name
        self.type = type?[0]
    }
}

class DotRequest : Node {
    var node : Node
    var parts : [Part]

    init(_ node : Node, _ parts : [Part]) {
        self.node = node
        self.parts = parts
    }
}

class MethodDeclaration : Node {
    var parts : [Part]
    var returnType : Node?
    var annotations : [Node]
    var body : [Node]

    init(_ parts : [Part], _ returnType : [Node]?, _ annotations : [Node]?, _ body : [Node]) {
        self.parts = parts
        self.returnType = returnType?[0]
        self.annotations = annotations ?? []
        self.body = body
    }
}

class Part : Node {
    var name : String
    var parameters : [Node]

    init(_ name : String, _ parameters : [Node]?) {
        self.name = name
        self.parameters = parameters ?? []
    }
}

class Assign : Node {
    var target : Node
    var value : Node

    init(_ node : Node, _ value : Node) {
        self.target = node
        self.value = value
    }
}

class ReturnStatement : Node {
    var value : Node

    init(_ value : Node) {
        self.value = value
    }
}

class Block : Node {
    var params : [Node]
    var body : [Node]

    init(_ params : [Node]?, _ body : [Node]) {
        self.params = params ?? []
        self.body = body
    }
}

class ImportStatement : Node {
    var name : String
    var binding : Node

    init(_ name : String, _ binding : Node) {
        self.name = name
        self.binding = binding
    }
}

class Comment : Node {
    var value : String

    init(_ value : String) {
        self.value = value
    }
}

func one(_ node : Node) -> [Node] {
    return [node]
}

let no : [Node] = []

func cons(_ head: Node, _ tail: [Node]) -> [Node] {
    return [head] + tail
}

func objCons(_ body : [Node], _ anns : [Node]?) -> Node {
    return ObjectConstructor(body, anns ?? [])
}


func defDec(_ name : String, _ type : [Node]?, _ anns : [Node]?, _ value : Node) -> Node {
    return DefDeclaration(name, type?[0], anns ?? [], value)
}

func varDec(_ name : String, _ type : [Node]?, _ anns : [Node]?, _ value : [Node]?) -> Node {
    let val : Node? =
        if value == nil || value?.count == 0 {
            nil
        } else {
            value?[0]
        }
    return VarDeclaration(name, type?[0], anns ?? [], val)
}

func numLit(_ value : Int) -> Node {
    return NumberLiteral(value)
}

func strLit(_ value : String) -> Node {
    return StringLiteral(value)
}

func interpStr(_ before : String, _ expression : Node, _ after : Node) -> Node {
    return InterpolatedString(before, expression, after)
}

func lexReq(_ parts : [Node]) -> Node {
    return LexicalRequest(parts.compactMap { $0 as? Part })
}

func identifierDeclaration(_ name : String, _ type : [Node]?) -> Node {
    return IdentifierDeclaration(name, type)
}

func dotReq(_ node : Node, _ parts : [Node]?) -> Node {
    if parts == nil {
        return DotRequest(node, [])
    } else {
        return DotRequest(node, parts!.compactMap { $0 as? Part })
    }
}

func methDec(_ parts : [Node], _ returnType : [Node]?, _ anns : [Node]?, _ body : [Node]) -> Node {
    return MethodDeclaration(parts.compactMap { $0 as? Part }, returnType, anns, body)
}

func part(_ name : String, _ parameters : [Node]?) -> Node {
    return Part(name, parameters)
}

func assn(_ node : Node, _ value : Node) -> Node {
    return Assign(node, value)
}

func returnStmt(_ value : Node) -> Node {
    return ReturnStatement(value)
}

func block(_ params : [Node]?, _ body : [Node]) -> Node {
    return Block(params, body)
}

func importStmt(_ name : String, _ binding : Node) -> Node {
    return ImportStatement(name, binding)
}

func comment(_ value : String) -> Node {
    return Comment(value)
}

func safeStr(_ before : String, _ middle : String, _ after : String) -> String {
    return before + middle + after
}

let charDollar = "$";
let charBackslash = "\\";
let charDQuote = "\"";
let charLF = "\n";
let charCR = "\r";
let charLBrace = "{";
let charStar = "*";
let charTilde = "~";
let charBacktick = "`";
let charCaret = "^";
let charAt = "@";
let charPercent = "%";
let charAmp = "&";
let charHash = "#";
let charExclam = "!";

let program = objCons(cons(importStmt("ast", identifierDeclaration("ast", nil)), cons(comment(" This file makes use of all AST nodes"), cons(defDec("x", nil, nil, objCons(cons(varDec("y", one(lexReq(one(part("Number", nil)))), nil, one(numLit(1))), one(methDec(cons(part("foo", one(identifierDeclaration("arg", one(lexReq(one(part("Action", nil))))))), one(part("bar", one(identifierDeclaration("n", nil))))), one(lexReq(one(part("String", nil)))), nil, cons(assn(dotReq(lexReq(one(part("self", nil))), one(part("y", nil))), dotReq(dotReq(lexReq(one(part("arg", nil))), one(part("apply", nil))), one(part("+", one(lexReq(one(part("n", nil)))))))), one(returnStmt(interpStr(safeStr("y ", charAt, " "), lexReq(one(part("y", nil))), strLit(safeStr("", charExclam, ""))))))))), nil)), one(lexReq(one(part("print", one(dotReq(lexReq(one(part("x", nil))), cons(part("foo", one(block(nil, one(numLit(2))))), one(part("bar", one(numLit(3)))))))))))))), nil)

func escapeString(_ str: String) -> String {
    return str
        .replacing("\\", with: "\\\\")
        .replacing("\"", with: "\\\"")
        .replacing("\n", with: "\\n")
        .replacing("\r", with: "\\r")
        .replacing("\t", with: "\\t")
        .replacing("{", with: "\\{")
}

func prettyPrint(_ node: Node) -> String {
    return prettyPrint(node: node, depth: 0)
}

func prettyPrint(node: Node, depth: Int) -> String {
    switch node {
    case let obj as ObjectConstructor:
        let bodyIndent = String(repeating: " ", count: depth + 4)
        return "object {\n" +
            obj.body.map { bodyIndent + prettyPrint(node: $0, depth: depth + 4) }.joined(separator: "\n") +
            "\n" + String(repeating: " ", count: depth) + "}"
    case let def as DefDeclaration:
        return "def \(def.name)" +
            (def.type != nil ? " : \(prettyPrint(node: def.type!, depth: depth))" : "") +
            (def.annotations.count > 0 ? " " + def.annotations.map { prettyPrint(node: $0, depth: depth) }.joined(separator: " ") : "") +
            " = " + prettyPrint(node: def.value, depth: depth)
    case let varDec as VarDeclaration:
        return "var \(varDec.name)" +
            (varDec.type != nil ? " : \(prettyPrint(node: varDec.type!, depth: depth))" : "") +
            (varDec.annotations.count > 0 ? " " + varDec.annotations.map { prettyPrint(node: $0, depth: depth) }.joined(separator: " ") : "") +
            (varDec.value != nil ? " := \(prettyPrint(node: varDec.value!, depth: depth))" : "")
    case let num as NumberLiteral:
        return "\(num.value)"
    case let str as StringLiteral:
        return "\"\(str.value)\""
    case let interp as InterpolatedString:
        var parts = [escapeString(interp.before)]
        parts.append("{" + prettyPrint(node: interp.expression, depth: depth) + "}")
        var rest = interp.after
        while true {
            switch rest {
                case let str as StringLiteral:
                    parts.append(escapeString(str.value))
                    return "\"" + parts.joined() + "\""
                case let interp as InterpolatedString:
                    parts.append(escapeString(interp.before))
                    parts.append("{" + prettyPrint(node: interp.after, depth: depth) + "}")
                    rest = interp.after
                default:
                    return "\"" + parts.joined() + "\""
            }
        }
    case let lex as LexicalRequest:
        return lex.parts.map {
            var args : String = ""
            if ($0.parameters.count > 1) {
                args = "(" + $0.parameters.map { prettyPrint(node: $0, depth: depth) }.joined(separator: ", ") + ")"
            } else if ($0.parameters.count > 0) {
                args = switch $0.parameters[0] {
                    case let str as StringLiteral:
                        " " + prettyPrint(node: str, depth: depth)
                    case let interp as InterpolatedString:
                        " " + prettyPrint(node: interp, depth: depth)
                    case let num as NumberLiteral:
                        " " + prettyPrint(node: num, depth: depth)
                    case let block as Block:
                        " " + prettyPrint(node: block, depth: depth)
                    default:
                        "(\(prettyPrint(node: $0.parameters[0], depth: depth)))"
                }
            }
            return $0.name + args
        }.joined(separator: " ") 
    case let id as IdentifierDeclaration:
        return "\(id.name)" + (id.type != nil ? ": \(prettyPrint(node: id.type!, depth: depth))" : "")
    case let dot as DotRequest:
        if dot.parts.count == 1 {
            let first = dot.parts[0]
            switch first.name {
                case "+", "-", "*", "/", "%", "==", "!=", "<", "<=", ">", ">=":
                    return prettyPrint(node: dot.node, depth: depth) + " " + first.name + " " + prettyPrint(node: first.parameters[0], depth: depth)
                case "prefix!", "prefix-":
                    return first.name + prettyPrint(node: dot.node, depth: depth)
                default: break
            }
        }

        return prettyPrint(node: dot.node, depth: depth) + "." + dot.parts.map {
            var args : String = ""
            if ($0.parameters.count > 1) {
                args = "(" + $0.parameters.map { prettyPrint(node: $0, depth: depth) }.joined(separator: ", ") + ")"
            } else if ($0.parameters.count > 0) {
                args = switch $0.parameters[0] {
                    case let str as StringLiteral:
                        " " + prettyPrint(node: str, depth: depth)
                    case let interp as InterpolatedString:
                        " " + prettyPrint(node: interp, depth: depth)
                    case let num as NumberLiteral:
                        " " + prettyPrint(node: num, depth: depth)
                    case let block as Block:
                        " " + prettyPrint(node: block, depth: depth)
                    default:
                        "(\(prettyPrint(node: $0.parameters[0], depth: depth)))"
                }
            }
            return $0.name + args
        }.joined(separator: " ") 
    case let meth as MethodDeclaration:
        let parts = meth.parts.map {
            if ($0.parameters.count > 0) {
                return "\($0.name)(\(prettyPrint(node: $0.parameters[0], depth: depth)))"
            }
            return $0.name
        }.joined(separator: " ")
        let retAnn = (meth.returnType != nil ? " -> \(prettyPrint(node: meth.returnType!, depth: depth))" : "") +
            (meth.annotations.count > 0 ? " is " + meth.annotations.map { prettyPrint(node: $0, depth: depth) }.joined(separator: " ") : "")
        let bodyIndent = String(repeating: " ", count: depth + 4)
        let body = (meth.body.count > 0 ? " {\n" +
            meth.body.map { bodyIndent + prettyPrint(node: $0, depth: depth + 4) }.joined(separator: "\n") +
            "\n" + String(repeating: " ", count: depth) + "}" : "")
        return "method \(parts)\(retAnn)" + body
    case let part as Part:
        return "\(part.name)" +
            (part.parameters.count > 0 ? "(" + part.parameters.map { prettyPrint(node: $0, depth: depth) }.joined(separator: ", ") + ")" : "")
    case let assign as Assign:
        return "\(prettyPrint(node: assign.target, depth: depth)) := \(prettyPrint(node: assign.value, depth: depth))"
    case let ret as ReturnStatement:
        return "return \(prettyPrint(node: ret.value, depth: depth))"
    case let block as Block:
        let params = if block.params.count > 0 { block.params.map { prettyPrint(node: $0, depth: depth + 4) }.joined(separator: ",") + " -> " } else { "" }
        let bodyIndent = String(repeating: " ", count: depth + 4)
        let bodyLines = block.body.map { prettyPrint(node: $0, depth: depth + 4) }
        if bodyLines.count == 1 {
            return "{ " + params + bodyLines[0] + " }"
        }
        return "{" +
            params +
            "\n" + bodyLines.map { bodyIndent + $0 }.joined(separator: "\n") +
            "\n" + String(repeating: " ", count: depth) + "}"
    case let imp as ImportStatement:
        return "import \"\(imp.name)\" as \(prettyPrint(node: imp.binding, depth: depth))"
    case let com as Comment:
        return "//\(com.value)"
    default:
        return "unknown node"
    }
}
print(prettyPrint(program))