#include "ast.h"
#include "grace.h"
#include "gc.h"

#include <stdio.h>
#include <stdlib.h>

static GraceObject *evaluate_module(char *name, ASTNode *ast, Env *env) {
    CaptureCont cc;
    cc.base.apply = capture_apply;
    cc.result     = grace_done;
    cc.base.gc_trace = NULL;  /* stack-local, no GC trace needed */
    cc.base.cleanup = NULL;
    cc.base.refcount = CONT_REFCOUNT_STATIC;
    cc.base.consumed = 0;
    trampoline(eval_node(ast, env, (Cont *)&cc));
    grace_register_module(name, cc.result);
    return cc.result;
}

int main() {
    GraceObject *prelude = make_prelude();
    gc_push_root(&prelude);
    Env *env = env_new(prelude);

    ASTNode *program;
    program = o0C(c0N(c0M(" Algol Bulletin, issue 17, Jul. 1964. Letter by Donald Knuth, p7."),c2N(m0D(o1N(p0T("A",c0N(i0D("k",nil),c0N(i0D("x1",nil),c0N(i0D("x2",nil),c0N(i0D("x3",nil),c2N(i0D("x4",nil),i0D("x5",nil)))))),nil)),nil,nil,c0N(v4R("k'",nil,nil,o1N(l0R("k(0)",nil,nil))),c0N(v4R("aRet",nil,nil,nil),c0N(d3F("B",nil,nil,b1K(nil,c0N(v4R("bRet",nil,nil,nil),c0N(a5N(l0R("k'(0)",nil,nil),d0R(l0R("k'(0)",nil,nil),"-(1)",o1N(n0M(1)),nil)),c0N(a5N(l0R("aRet(0)",nil,nil),l0R("A(6)",c0N(l0R("k'(0)",nil,nil),c0N(l0R("B(0)",nil,nil),c0N(l0R("x1(0)",nil,nil),c0N(l0R("x2(0)",nil,nil),c2N(l0R("x3(0)",nil,nil),l0R("x4(0)",nil,nil)))))),nil)),c2N(a5N(l0R("bRet(0)",nil,nil),l0R("aRet(0)",nil,nil)),l0R("bRet(0)",nil,nil))))))),c2N(l0R("if(1)then(1)else(1)",c0N(d0R(l0R("k'(0)",nil,nil),"<=(1)",o1N(n0M(0)),nil),c2N(b1K(nil,o1N(a5N(l0R("aRet(0)",nil,nil),d0R(d0R(l0R("x4(0)",nil,nil),"apply(0)",nil,nil),"+(1)",o1N(d0R(l0R("x5(0)",nil,nil),"apply(0)",nil,nil)),nil)))),b1K(nil,o1N(d0R(l0R("B(0)",nil,nil),"apply(0)",nil,nil))))),nil),l0R("aRet(0)",nil,nil)))))),l0R("print(1)",o1N(l0R("A(6)",c0N(n0M(10),c0N(b1K(nil,o1N(n0M(1))),c0N(b1K(nil,o1N(d0R(n0M(1),"prefix-(0)",nil,nil))),c0N(b1K(nil,o1N(d0R(n0M(1),"prefix-(0)",nil,nil))),c2N(b1K(nil,o1N(n0M(1))),b1K(nil,o1N(n0M(0)))))))),nil)),nil))),nil);

    evaluate_module("", program, env);

    return 0;
}
