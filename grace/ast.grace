import "nodes" as nodes

method cons(hd, tl) {
    nodes.cons(hd, tl)
}
method nil { nodes.nil }
method one(v) { nodes.one(v) }
method numberNode(n) { nodes.numLit(n) }
method stringNode(s) { nodes.strLit(s) }
method interpString(a, b, c) { nodes.interpStr(a, b, c) }
method block(params, body) { nodes.block(params, body) }
method defDecl(nm, dtype, anns, val) { nodes.defDec(nm, dtype, anns, val) }
method varDecl(nm, dtype, anns, val) { nodes.varDec(nm, dtype, anns, val) }
method lexicalRequest(pos, parts) { nodes.lexReq(parts) }
method explicitRequest(receiver, parts) { nodes.dotReq(receiver, parts) }
method explicitRequest(pos, receiver, parts) { nodes.dotReq(receiver, parts) }
method part(nm, args) { nodes.part(nm, args, nodes.nil) }
method part(nm, args, typeargs) { nodes.part(nm, args, typeargs) }
method methodDecl(pts, dtype, anns, bd) { nodes.methDec(pts, dtype, anns, bd) }
method objectConstructor(body, anns) { nodes.objCons(body, anns) }
method assign(lhs, rhs) { nodes.assn(lhs, rhs) }
method returnStmt(val) { nodes.returnStmt(val) }
method lineup(elems) { nodes.lineup(elems) }
method identifierDeclaration(nm, dtype) { nodes.identifierDeclaration(nm, dtype) }
method comment(text) { nodes.comment(text) }
method importStmt(source, bnd) { nodes.importStmt(source, bnd) }
method dialectStmt(source) { nodes.dialectStmt(source) }
