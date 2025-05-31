import "collections" as collections
import "objects" as objects
import "nodes" as nodes

method nil { nodes.nil }

method one(item) { nodes.one(item) }

method cons(head, list) { nodes.cons(head, list) }

method strLit(val) { nodes.strLit(val) }

method numLit(val) { nodes.numLit(val) }

method part(nm, args) { nodes.part(nm, args) }

method lexReq(partList) { nodes.lexReq(partList) }

method dotReq(rec, partList) { nodes.dotReq(rec, partList) }

method assn(lhs, val) { nodes.assn(lhs, val) }

method varDec(nm, dtype, anns, val) { nodes.varDec(nm, dtype, anns, val) }

method defDec(nm, dtype, anns, val) { nodes.defDec(nm, dtype, anns, val) }

method identifierDeclaration(nm, dtype) { nodes.identifierDeclaration(nm, dtype) }

method methDec(pts, dtype, anns, bd) { nodes.methDec(pts, dtype, anns, bd) }

method importStmt(src, bnd) { nodes.importStmt(src, bnd) }

method comment(text) { nodes.comment(text) }

method interpStr(beforeStr, expr, afterNode) { nodes.interpStr(beforeStr, expr, afterNode) }

method returnStmt(val) { nodes.returnStmt(val)  }

method block(params, bd) { nodes.block(params, bd) }

method objCons(bd, anns) { nodes.objCons(bd, anns) }

def charDollar = "$";
def charBackslash = "\\";
def charDQuote = "\"";
def charLF = "\n";
def charCR = "\r";
def charLBrace = "\{";
def charStar = "*";
def charTilde = "~";
def charBacktick = "`";
def charCaret = "^";
def charAt = "@";
def charPercent = "%";
def charAmp = "&";
def charHash = "#";
def charExclam = "!";

method safeStr(before, char, after) {
    return before ++ char ++ after
}

def program = objCons(cons(defDec("greeter", nil, nil, objCons(one(methDec(one(part("greeting", one(identifierDeclaration("nm", nil)))), nil, nil, one(interpStr("Hello, ", lexReq(one(part("nm", nil))), strLit(safeStr("", charExclam, "")))))), nil)), cons(varDec("s", nil, nil, one(strLit("world"))), cons(assn(lexReq(one(part("s", nil))), dotReq(lexReq(one(part("greeter", nil))), one(part("greeting", one(lexReq(one(part("s", nil)))))))), one(lexReq(one(part("print", one(lexReq(one(part("s", nil))))))))))), nil)

program.evaluate(nodes.evaluationContext(objects.terminalObject))