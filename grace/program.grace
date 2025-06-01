dialect "grast"

def program = objCons(cons(methDec(one(part("foo", nil)), nil, nil, one(lexReq(one(part("bar", one(block(nil, one(returnStmt(numLit(5)))))))))), cons(methDec(one(part("bar", one(identifierDeclaration("blk", nil)))), nil, nil, one(dotReq(lexReq(one(part("blk", nil))), one(part("apply", nil))))), one(lexReq(one(part("print", one(lexReq(one(part("foo", nil)))))))))), nil)

run(program)
