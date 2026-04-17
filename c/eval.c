/* CPS evaluator for Grace AST nodes */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "grace.h"
#include "gc.h"

/*
 * parts_to_name: build method name like "at(1)put(1)" from NK_PART list
 */
char *parts_to_name(ASTNode *parts) {
    char *name = str_dup("");
    for (ASTNode *p = parts; p != NULL; p = p->a2) {
        if (p->kind != NK_CONS) break;
        ASTNode *part = p->a1;
        if (!part || part->kind != NK_PART) break;
        /* part->strval = part name, part->a1 = params list */
        int n = list_length(part->a1);  /* number of parameters */
        char *seg = str_fmt("%s(%d)", part->strval, n);
        char *tmp = str_cat(name, seg);
        free(seg);
        free(name);
        name = tmp;
    }
    return name;
}

/* Convert a getter name like "foo(0)" to a setter name "foo:=(1)" by
 * stripping the argument-count suffix before appending ":=(1)". */
static char *getter_to_setter(const char *getter) {
    const char *p = strchr(getter, '(');
    if (p)
        return str_fmt("%.*s:=(1)", (int)(p - getter), getter);
    return str_fmt("%s:=(1)", getter);
}

/*
 * EvalArgsCont: evaluate a flat ASTNode arg list one by one into an array,
 * then dispatch a method call.
 */
typedef struct EvalArgsCont {
    Cont         base;
    ASTNode     *rem;        /* next cons cell to evaluate */
    GraceObject *recv;       /* receiver for the eventual call */
    Env         *env;
    const char  *name;       /* method name */
    GraceObject **arr;       /* accumulator */
    int          idx;        /* how many filled */
    int          total;
    Cont        *final_k;
} EvalArgsCont;

static PendingStep *eac_apply(Cont *c, GraceObject *v) {
    EvalArgsCont *ea = (EvalArgsCont *)c;
    ea->arr[ea->idx++] = v;
    ea->rem = ea->rem->a2;
    if (ea->idx == ea->total || ea->rem == NULL) {
        GraceObject *recv = ea->recv;
        Env *env = env_retain(ea->env);
        const char *name = ea->name;
        GraceObject **arr = ea->arr;
        int idx = ea->idx;
        Cont *k = cont_retain(ea->final_k);
        ea->name = NULL;
        ea->arr  = NULL;
        cont_consumed(c);
        PendingStep *r = grace_request(recv, env, name, arr, idx, k);
        env_release(env);
        cont_release(k);
        free(arr);
        free((char *)name);
        return r;
    }
    return eval_node(ea->rem->a1, ea->env, c);
}

static void eac_trace(Cont *c) {
    EvalArgsCont *ea = (EvalArgsCont *)c;
    gc_mark_grey(ea->recv);
    gc_trace_env(ea->env);
    if (ea->arr) {
        for (int i = 0; i < ea->idx; i++)
            gc_mark_grey(ea->arr[i]);
    }
    gc_trace_cont(ea->final_k);
}
static void eac_cleanup(Cont *c) {
    EvalArgsCont *ea = (EvalArgsCont *)c;
    env_release(ea->env);
    cont_release_abandon(ea->final_k);
    free(ea->arr);
    free((char *)ea->name);
}

/*
 * LineupBuildCont: evaluate a cons-list of element nodes into an array,
 * then construct a GraceLineup.
 */
typedef struct LineupBuildCont {
    Cont         base;
    ASTNode     *rem;    /* next cons cell to evaluate */
    GraceObject **arr;   /* accumulator */
    int          idx;    /* how many filled */
    int          total;
    Env         *env;
    Cont        *k;
} LineupBuildCont;

static PendingStep *lineup_build_apply(Cont *c, GraceObject *v) {
    LineupBuildCont *lb = (LineupBuildCont *)c;
    lb->arr[lb->idx++] = v;
    lb->rem = lb->rem->a2;
    if (lb->idx == lb->total || lb->rem == NULL) {
        GraceObject **arr = lb->arr;
        int n = lb->idx;
        Cont *k = cont_retain(lb->k);
        lb->arr = NULL;
        cont_consumed(c);
        PendingStep *r = cont_apply(k, grace_lineup_new(arr, n));
        cont_release(k);
        return r;
    }
    return eval_node(lb->rem->a1, lb->env, c);
}

static void lineup_build_trace(Cont *c) {
    LineupBuildCont *lb = (LineupBuildCont *)c;
    gc_trace_env(lb->env);
    if (lb->arr) {
        for (int i = 0; i < lb->idx; i++)
            gc_mark_grey(lb->arr[i]);
    }
    gc_trace_cont(lb->k);
}

static void lineup_build_cleanup(Cont *c) {
    LineupBuildCont *lb = (LineupBuildCont *)c;
    env_release(lb->env);
    cont_release_abandon(lb->k);
    free(lb->arr);
}

/*
 * Method declaration closure
 */
typedef struct {
    ASTNode     *parts;       /* NK_CONS list of NK_PART */
    ASTNode     *body;        /* NK_CONS list of statements */
    GraceObject *lex_scope;   /* captured outer scope */
    GraceObject *lex_self;    /* captured self */
} MethodClosure;

static void method_closure_trace(void *data) {
    MethodClosure *mc = (MethodClosure *)data;
    gc_mark_grey(mc->lex_scope);
    gc_mark_grey(mc->lex_self);
}
static void method_closure_free(void *data) {
    free(data);
}

/* Forward declarations for inheritance support */
static PendingStep *method_inherit_fn(GraceObject *self, Env *env,
                                       GraceObject **args, int nargs, Cont *k,
                                       void *data);
static PendingStep *eval_objcons_inherit(ASTNode *node, Env *env,
                                          GraceObject *inherit_obj, Cont *k);

/* Check if the last statement of a body cons list is NK_OBJCONS */
static int body_tail_is_objcons(ASTNode *body) {
    if (!body) return 0;
    while (body->kind == NK_CONS) {
        if (!body->a2) return body->a1 && body->a1->kind == NK_OBJCONS;
        body = body->a2;
    }
    return body->kind == NK_OBJCONS;
}

/* Get the last element of a cons list */
static ASTNode *cons_last(ASTNode *list) {
    if (!list) return NULL;
    while (list->kind == NK_CONS) {
        if (!list->a2) return list->a1;
        list = list->a2;
    }
    return list;
}

/* Build a cons list of all elements except the last.
 * Returns NULL if list has 0 or 1 elements.
 * Allocates new NK_CONS cells but shares original sub-nodes. */
static ASTNode *cons_init(ASTNode *list) {
    if (!list) return NULL;
    if (list->kind != NK_CONS) return NULL;
    if (!list->a2) return NULL;
    ASTNode *rest = cons_init(list->a2);
    ASTNode *n = malloc(sizeof(ASTNode));
    memset(n, 0, sizeof(*n));
    n->kind = NK_CONS;
    n->a1 = list->a1;
    n->a2 = rest;
    return n;
}

/* Helper: set trace_data/free_data on the last method entry of a user object */
static void set_last_method_gc(GraceObject *obj, void (*trace)(void*), void (*freefn)(void*)) {
    GraceUserObject *uo = (GraceUserObject *)obj;
    MethodEntry *last = uo->methods;
    if (!last) return;
    while (last->next) last = last->next;
    last->trace_data = trace;
    last->free_data  = freefn;
}

static PendingStep *method_fn(GraceObject *self, Env *env,
                               GraceObject **args, int nargs, Cont *k,
                               void *data) {
    (void)env;
    MethodClosure *mc = (MethodClosure *)data;

    /* Build frame scope */
    GraceObject *frame = grace_user_new(mc->lex_scope);

    /* Bind 'self' */
    user_bind_def(frame, "self", self);

    /* Walk parts, binding params */
    int ai = 0;
    for (ASTNode *pl = mc->parts; pl != NULL; pl = pl->a2) {
        if (pl->kind != NK_CONS) break;
        ASTNode *part = pl->a1;
        if (!part || part->kind != NK_PART) break;
        for (ASTNode *pm = part->a1; pm != NULL; pm = pm->a2) {
            if (pm->kind != NK_CONS) break;
            ASTNode *decl = pm->a1;
            if (!decl) break;
            const char *pname = decl->strval;
            GraceObject *val = (ai < nargs) ? args[ai] : grace_done;
            user_bind_def(frame, pname, val);
            ai++;
        }
    }

    /* Build env for method body - return_k = caller's k */
    Env *inner = malloc(sizeof(Env));
    inner->refcount = 1;
    inner->receiver = self;    /* methods execute with defining self as receiver */
    inner->scope    = frame;
    inner->return_k = cont_retain(k);       /* return stmt goes to caller's k */
    inner->except_k = cont_retain(env->except_k);
    inner->reset_k  = cont_retain(env->reset_k);

    PendingStep *r = eval_stmts(mc->body, inner, k);
    env_release(inner);
    return r;
}

/*
 * eval_stmts - evaluate a cons list of statements
 */
typedef struct {
    Cont         base;
    ASTNode     *rest;
    Env         *env;
    Cont        *k;
} StmtsCont;

static PendingStep *stmts_cont_apply(Cont *c, GraceObject *val) {
    (void)val;
    StmtsCont *sc = (StmtsCont *)c;
    ASTNode *rest = sc->rest;
    Env *env = env_retain(sc->env);
    Cont *k = cont_retain(sc->k);
    cont_consumed(c);
    PendingStep *r = eval_stmts(rest, env, k);
    env_release(env);
    cont_release(k);
    return r;
}
static void stmts_cont_trace(Cont *c) {
    StmtsCont *sc = (StmtsCont *)c;
    gc_trace_env(sc->env);
    gc_trace_cont(sc->k);
}
static void stmts_cont_cleanup(Cont *c) {
    StmtsCont *sc = (StmtsCont *)c;
    env_release(sc->env);
    cont_release_abandon(sc->k);
}

PendingStep *eval_stmts(ASTNode *stmts, Env *env, Cont *k) {
    if (stmts == NULL)
        return cont_apply(k, grace_done);

    if (stmts->kind == NK_CONS) {
        ASTNode *head = stmts->a1;
        ASTNode *rest = stmts->a2;
        if (rest == NULL) {
            return eval_node(head, env, k);
        }
        StmtsCont *sc = CONT_ALLOC(StmtsCont);
        sc->base.apply = stmts_cont_apply;
        sc->base.gc_trace = stmts_cont_trace;
        sc->base.cleanup = stmts_cont_cleanup;
        sc->rest = rest;
        sc->env  = env_retain(env);
        sc->k    = cont_retain(k);
        return eval_node(head, env, (Cont *)sc);
    }
    /* Single node (shouldn't occur but be safe) */
    return eval_node(stmts, env, k);
}

/*
 * Dot request continuation - eval receiver, then (optionally) eval args
 */

/* After evaluating all args for a dot/lex req */
typedef struct {
    GraceObject *recv;
    Env         *env;
    const char  *name;
    Cont        *k;
} CallData;

// static PendingStep *call_done(GraceObject **args, int nargs, void *data) {
//     CallData *cd = (CallData *)data;
//     return grace_request(cd->recv, cd->env, cd->name, args, nargs, cd->k);
// }

/* After evaluating receiver of dot req, evaluate args then call */
typedef struct {
    Cont     base;
    ASTNode *flat_args;   /* flat arg nodes */
    Env     *env;
    char    *name;
    Cont    *k;
} DotRecvCont;

static PendingStep *dot_recv_cont_apply(Cont *c, GraceObject *recv) {
    DotRecvCont *dc = (DotRecvCont *)c;
    int total = list_length(dc->flat_args);
    if (total == 0) {
        Env *env = env_retain(dc->env);
        char *name = dc->name;
        Cont *k = cont_retain(dc->k);
        dc->name = NULL;
        cont_consumed(c);
        PendingStep *r = grace_request(recv, env, name, NULL, 0, k);
        env_release(env);
        cont_release(k);
        free(name);
        return r;
    }
    EvalArgsCont *eac = CONT_ALLOC(EvalArgsCont);
    eac->base.apply = eac_apply;
    eac->base.gc_trace = eac_trace;
    eac->base.cleanup = eac_cleanup;
    eac->rem     = dc->flat_args;
    eac->recv    = recv;
    eac->env     = env_retain(dc->env);
    eac->name    = dc->name;
    eac->arr     = malloc(total * sizeof(GraceObject *));
    eac->idx     = 0;
    eac->total   = total;
    eac->final_k = cont_retain(dc->k);
    ASTNode *first = dc->flat_args->a1;
    Env *env = env_retain(dc->env);
    dc->name = NULL;
    cont_consumed(c);
    PendingStep *r = eval_node(first, env, (Cont *)eac);
    env_release(env);
    return r;
}

static void dot_recv_cont_trace(Cont *c) {
    DotRecvCont *dc = (DotRecvCont *)c;
    gc_trace_env(dc->env);
    gc_trace_cont(dc->k);
}
static void dot_recv_cont_cleanup(Cont *c) {
    DotRecvCont *dc = (DotRecvCont *)c;
    env_release(dc->env);
    cont_release_abandon(dc->k);
    free(dc->name);
}

/*
 * flatten_args: collect all args from a parts list into a flat ASTNode cons
 * list (in order: part0 args, part1 args, ...)
 */
// static ASTNode *flatten_args(ASTNode *parts) {
//     /* Build flat list preserving order using a tail pointer */
//     ASTNode *head = NULL;
//     ASTNode **tail = &head;
//     for (ASTNode *pl = parts; pl != NULL; pl = pl->a2) {
//         if (pl->kind != NK_CONS) break;
//         ASTNode *part = pl->a1;
//         if (!part || part->kind != NK_PART) break;
//         for (ASTNode *al = part->a1; al != NULL; al = al->a2) {
//             if (al->kind != NK_CONS) break;
//             ASTNode *cell = malloc(sizeof(ASTNode));
//             cell->kind = NK_CONS;
//             cell->a1   = al->a1;
//             cell->a2   = NULL;
//             *tail = cell;
//             tail  = &cell->a2;
//         }
//     }
//     return head;
// }

/*
 * Interp-string continuation chain
 */
typedef struct {
    Cont         base;
    const char  *prefix;
    ASTNode     *next_interp;  /* rest of interp */
    Env         *env;
    Cont        *outer_k;
    const char  *accum;
} InterpAsStrCont;

static void interp_asstr_cont_trace(Cont *c);  /* forward declaration */
static void interp_asstr_cont_cleanup(Cont *c);  /* forward declaration */

typedef struct {
    Cont         base;
    InterpAsStrCont *parent;
} InterpValCont;

/* Receives an evaluated interp expression and calls asString(0) */
typedef struct { Cont base; InterpAsStrCont *nc; } InterpExprCont;
static PendingStep *interp_expr_cont_apply(Cont *c, GraceObject *v) {
    InterpExprCont *ie = (InterpExprCont *)c;
    InterpAsStrCont *nc = ie->nc;
    cont_retain((Cont *)nc);
    cont_consumed(c);
    PendingStep *r = grace_request(v, nc->env, "asString(0)", NULL, 0, (Cont *)nc);
    cont_release((Cont *)nc);
    return r;
}
static void interp_expr_cont_trace(Cont *c) {
    InterpExprCont *ie = (InterpExprCont *)c;
    gc_trace_cont((Cont *)ie->nc);
}
static void interp_expr_cont_cleanup(Cont *c) {
    InterpExprCont *ie = (InterpExprCont *)c;
    cont_release_abandon((Cont *)ie->nc);
}

static PendingStep *interp_asstr_cont_apply(Cont *c, GraceObject *str_val) {
    InterpAsStrCont *ic = (InterpAsStrCont *)c;
    const char *s = grace_string_val(str_val);
    char *combined = str_cat(ic->accum, s);
    /* Continue with rest */
    if (ic->next_interp == NULL) {
        Cont *outer_k = cont_retain(ic->outer_k);
        cont_consumed(c);
        GraceObject *result = grace_string_new(combined);
        free(combined);
        PendingStep *r = cont_apply(outer_k, result);
        cont_release(outer_k);
        return r;
    }
    /* next interp: strval=prefix, a1=expr, a2=next */
    ASTNode *ni = ic->next_interp;
    char *np = str_cat(combined, ni->strval ? ni->strval : "");
    free(combined);
    /* If ni has no expression (e.g. it is a trailing strLit suffix), we're done */
    if (ni->a1 == NULL) {
        Cont *outer_k = cont_retain(ic->outer_k);
        cont_consumed(c);
        GraceObject *result = grace_string_new(np);
        free(np);
        PendingStep *r = cont_apply(outer_k, result);
        cont_release(outer_k);
        return r;
    }
    InterpAsStrCont *nc = CONT_ALLOC(InterpAsStrCont);
    nc->base.apply   = interp_asstr_cont_apply;
    nc->base.gc_trace = interp_asstr_cont_trace;
    nc->base.cleanup = interp_asstr_cont_cleanup;
    nc->prefix       = np;
    nc->next_interp  = ni->a2;  /* next interp segment */
    nc->env          = env_retain(ic->env);
    nc->outer_k      = cont_retain(ic->outer_k);
    nc->accum        = np;
    cont_consumed(c);
    /* Evaluate expr, then call asString(0) */
    InterpExprCont *as2 = CONT_ALLOC(InterpExprCont);
    as2->base.apply = interp_expr_cont_apply;
    as2->base.gc_trace = interp_expr_cont_trace;
    as2->base.cleanup = interp_expr_cont_cleanup;
    as2->nc = nc;
    cont_retain((Cont *)nc);
    return eval_node(ni->a1, nc->env, (Cont *)as2);
}

static void interp_asstr_cont_trace(Cont *c) {
    InterpAsStrCont *ic = (InterpAsStrCont *)c;
    gc_trace_env(ic->env);
    gc_trace_cont(ic->outer_k);
}
static void interp_asstr_cont_cleanup(Cont *c) {
    InterpAsStrCont *ic = (InterpAsStrCont *)c;
    free((char *)ic->accum);
    env_release(ic->env);
    cont_release_abandon(ic->outer_k);
}

/*
 * Object constructor continuation
 */
typedef struct {
    Cont         base;
    GraceObject *obj;
    Cont        *k;
} ObjDoneCont;

static PendingStep *objdone_apply(Cont *c, GraceObject *val) {
    (void)val;
    ObjDoneCont *oc = (ObjDoneCont *)c;
    GraceObject *obj = oc->obj;
    Cont *k = cont_retain(oc->k);
    cont_consumed(c);
    PendingStep *r = cont_apply(k, obj);
    cont_release(k);
    return r;
}
static void objdone_trace(Cont *c) {
    ObjDoneCont *oc = (ObjDoneCont *)c;
    gc_mark_grey(oc->obj);
    gc_trace_cont(oc->k);
}
static void objdone_cleanup(Cont *c) {
    ObjDoneCont *oc = (ObjDoneCont *)c;
    cont_release_abandon(oc->k);
}

/*
 * Inherit support: InheritTailCont, method_inherit_fn, eval_objcons_inherit
 */

/* After evaluating the prefix body of an inheritable method,
 * evaluate the tail NK_OBJCONS using the inheriting object. */
typedef struct {
    Cont         base;
    ASTNode     *tail_node;
    GraceObject *inheriting;
    Env         *env;
    Cont        *k;
} InheritTailCont;

static PendingStep *inherit_tail_apply(Cont *c, GraceObject *val) {
    (void)val;
    InheritTailCont *itc = (InheritTailCont *)c;
    ASTNode *tail = itc->tail_node;
    GraceObject *inheriting = itc->inheriting;
    Env *env = env_retain(itc->env);
    Cont *k = cont_retain(itc->k);
    cont_consumed(c);
    PendingStep *r = eval_objcons_inherit(tail, env, inheriting, k);
    env_release(env);
    cont_release(k);
    return r;
}
static void inherit_tail_trace(Cont *c) {
    InheritTailCont *itc = (InheritTailCont *)c;
    gc_mark_grey(itc->inheriting);
    gc_trace_env(itc->env);
    gc_trace_cont(itc->k);
}
static void inherit_tail_cleanup(Cont *c) {
    InheritTailCont *itc = (InheritTailCont *)c;
    env_release(itc->env);
    cont_release_abandon(itc->k);
}

/* Inherit-variant method: like method_fn but the last argument is the
 * inheriting object.  Evaluates the body prefix normally, then evaluates
 * the tail NK_OBJCONS using the inheriting object. */
static PendingStep *method_inherit_fn(GraceObject *self, Env *env,
                                       GraceObject **args, int nargs, Cont *k,
                                       void *data) {
    (void)env;
    MethodClosure *mc = (MethodClosure *)data;
    GraceObject *inheriting = args[nargs - 1];

    GraceObject *frame = grace_user_new(mc->lex_scope);
    user_bind_def(frame, "self", self);

    /* Bind params from original parts (excluding the trailing inherit arg) */
    int ai = 0;
    for (ASTNode *pl = mc->parts; pl != NULL; pl = pl->a2) {
        if (pl->kind != NK_CONS) break;
        ASTNode *part = pl->a1;
        if (!part || part->kind != NK_PART) break;
        for (ASTNode *pm = part->a1; pm != NULL; pm = pm->a2) {
            if (pm->kind != NK_CONS) break;
            ASTNode *decl = pm->a1;
            if (!decl) break;
            const char *pname = decl->strval;
            GraceObject *val = (ai < nargs - 1) ? args[ai] : grace_done;
            user_bind_def(frame, pname, val);
            ai++;
        }
    }

    Env *inner = malloc(sizeof(Env));
    inner->refcount = 1;
    inner->receiver = self;
    inner->scope    = frame;
    inner->return_k = cont_retain(k);
    inner->except_k = cont_retain(env->except_k);
    inner->reset_k  = cont_retain(env->reset_k);

    ASTNode *tail = cons_last(mc->body);
    ASTNode *prefix = cons_init(mc->body);

    if (prefix) {
        InheritTailCont *itc = CONT_ALLOC(InheritTailCont);
        itc->base.apply    = inherit_tail_apply;
        itc->base.gc_trace = inherit_tail_trace;
        itc->base.cleanup  = inherit_tail_cleanup;
        itc->tail_node   = tail;
        itc->inheriting  = inheriting;
        itc->env         = env_retain(inner);
        itc->k           = cont_retain(k);
        PendingStep *r = eval_stmts(prefix, inner, (Cont *)itc);
        env_release(inner);
        return r;
    } else {
        PendingStep *r = eval_objcons_inherit(tail, inner, inheriting, k);
        env_release(inner);
        return r;
    }
}

/* Evaluate an object constructor body, building into an existing object
 * (inherit_obj) instead of creating a new one.  Methods already on the
 * object (from a more-derived level) are skipped during hoisting. */
static PendingStep *eval_objcons_inherit(ASTNode *node, Env *env,
                                          GraceObject *inherit_obj, Cont *k) {
    GraceUserObject *obj = (GraceUserObject *)inherit_obj;

    /* Inherit dialect from outer scope if not already set */
    GraceObject *outer = env->scope;
    if (outer && outer->vt == &grace_user_vtable && !obj->dialect)
        obj->dialect = ((GraceUserObject *)outer)->dialect;

    Env *inner = malloc(sizeof(Env));
    inner->refcount = 1;
    inner->receiver = (GraceObject *)obj;
    inner->scope    = (GraceObject *)obj;
    inner->return_k = cont_retain(cont_done);
    inner->except_k = cont_retain(env->except_k);
    inner->reset_k  = cont_retain(env->reset_k);

    user_bind_def((GraceObject *)obj, "self", (GraceObject *)obj);

    /* Pre-pass: hoist method and def declarations, skipping names
     * already on the object (overridden by the more-derived level). */
    for (ASTNode *n = node->a1; n; ) {
        ASTNode *stmt;
        if (n->kind == NK_CONS) { stmt = n->a1; n = n->a2; }
        else                    { stmt = n;     n = NULL;  }
        if (!stmt) continue;
        if (stmt->kind == NK_METH_DECL) {
            char *mname = parts_to_name(stmt->a1);
            if (!user_has_method((GraceObject *)obj, mname)) {
                MethodClosure *mc = malloc(sizeof(MethodClosure));
                mc->parts     = stmt->a1;
                mc->body      = stmt->a4;
                mc->lex_scope = (GraceObject *)obj;
                mc->lex_self  = (GraceObject *)obj;
                user_add_method((GraceObject *)obj, mname, method_fn, mc);
                set_last_method_gc((GraceObject *)obj, method_closure_trace, method_closure_free);
                if (body_tail_is_objcons(stmt->a4)) {
                    char *iname = str_fmt("%sinherit(1)", mname);
                    MethodClosure *imc = malloc(sizeof(MethodClosure));
                    imc->parts     = stmt->a1;
                    imc->body      = stmt->a4;
                    imc->lex_scope = (GraceObject *)obj;
                    imc->lex_self  = (GraceObject *)obj;
                    user_add_method((GraceObject *)obj, iname, method_inherit_fn, imc);
                    set_last_method_gc((GraceObject *)obj, method_closure_trace, method_closure_free);
                    free(iname);
                }
            }
            free(mname);
        } else if (stmt->kind == NK_DEF_DECL) {
            char *full = str_fmt("%s(0)", stmt->strval);
            if (!user_has_method((GraceObject *)obj, full))
                user_bind_def((GraceObject *)obj, stmt->strval, grace_uninit);
            free(full);
        }
    }

    ObjDoneCont *oc = CONT_ALLOC(ObjDoneCont);
    oc->base.apply  = objdone_apply;
    oc->base.gc_trace = objdone_trace;
    oc->base.cleanup = objdone_cleanup;
    oc->obj = (GraceObject *)obj;
    oc->k   = cont_retain(k);

    PendingStep *r = eval_stmts(node->a1, inner, (Cont *)oc);
    env_release(inner);
    return r;
}

/*
 * Use support: UseSourceCont - copy methods from source to target
 */
typedef struct {
    Cont  base;
    Env  *env;
    Cont *k;
} UseSourceCont;

static PendingStep *use_source_apply(Cont *c, GraceObject *source) {
    UseSourceCont *usc = (UseSourceCont *)c;
    Env *env = env_retain(usc->env);
    Cont *k = cont_retain(usc->k);
    cont_consumed(c);

    GraceObject *target = env->scope;

    if (!source || source->vt != &grace_user_vtable) {
        PendingStep *r = grace_raise(env, "UseError", "use: source must be an object");
        env_release(env);
        cont_release(k);
        return r;
    }

    GraceUserObject *src = (GraceUserObject *)source;

    /* Reject objects with mutable state (vars have :=(1) setters) */
    for (MethodEntry *m = src->methods; m; m = m->next) {
        if (strstr(m->name, ":=(")) {
            PendingStep *r = grace_raise(env, "UseError",
                "use: source object contains mutable state (var '%s')", m->name);
            env_release(env);
            cont_release(k);
            return r;
        }
    }

    /* Copy methods from source to target */
    for (MethodEntry *m = src->methods; m; m = m->next) {
        if (strcmp(m->name, "self(0)") == 0) continue;
        if (user_has_method(target, m->name)) continue;

        if (m->fn == method_fn || m->fn == method_inherit_fn) {
            /* Re-scope method closure to target */
            MethodClosure *mc = (MethodClosure *)m->data;
            MethodClosure *new_mc = malloc(sizeof(MethodClosure));
            new_mc->parts     = mc->parts;
            new_mc->body      = mc->body;
            new_mc->lex_scope = target;
            new_mc->lex_self  = target;
            user_add_method(target, m->name, m->fn, new_mc);
            set_last_method_gc(target, method_closure_trace, method_closure_free);
        } else {
            /* For def_fn etc.: copy as-is (data is GC-managed) */
            user_add_method(target, m->name, m->fn, m->data);
            GraceUserObject *tuo = (GraceUserObject *)target;
            MethodEntry *last = tuo->methods;
            while (last->next) last = last->next;
            last->trace_data = m->trace_data;
            /* free_data = NULL: don't double-free shared data */
        }
    }

    PendingStep *r = cont_apply(k, grace_done);
    env_release(env);
    cont_release(k);
    return r;
}
static void use_source_trace(Cont *c) {
    UseSourceCont *usc = (UseSourceCont *)c;
    gc_trace_env(usc->env);
    gc_trace_cont(usc->k);
}
static void use_source_cleanup(Cont *c) {
    UseSourceCont *usc = (UseSourceCont *)c;
    env_release(usc->env);
    cont_release_abandon(usc->k);
}

/*
 * NK_DEF_DECL continuation: after eval value, bind it
 */
typedef struct {
    Cont         base;
    const char  *name;
    GraceObject *target;  /* receiver to bind on */
    Cont        *k;
} DefBindCont;

static PendingStep *defbind_apply(Cont *c, GraceObject *val) {
    DefBindCont *dc = (DefBindCont *)c;
    GraceObject *target = dc->target;
    const char *name = dc->name;
    Cont *k = cont_retain(dc->k);
    cont_consumed(c);
    user_bind_def(target, name, val);
    PendingStep *r = cont_apply(k, grace_done);
    cont_release(k);
    return r;
}
static void defbind_trace(Cont *c) {
    DefBindCont *dc = (DefBindCont *)c;
    gc_mark_grey(dc->target);
    gc_trace_cont(dc->k);
}
static void defbind_cleanup(Cont *c) {
    DefBindCont *dc = (DefBindCont *)c;
    cont_release_abandon(dc->k);
}

/*
 * NK_VAR_DECL: bind variable with mutable cell; optionally eval init value
 */
typedef struct {
    Cont           base;
    GraceObject  **cell;
    Cont          *k;
} VarInitCont;

static PendingStep *varinit_apply(Cont *c, GraceObject *val) {
    VarInitCont *vc = (VarInitCont *)c;
    GraceObject **cell = vc->cell;
    Cont *k = cont_retain(vc->k);
    cont_consumed(c);
    gc_write_barrier(val);  /* cell may be in a BLACK-owned context */
    *cell = val;
    PendingStep *r = cont_apply(k, grace_done);
    cont_release(k);
    return r;
}
static void varinit_trace(Cont *c) {
    VarInitCont *vc = (VarInitCont *)c;
    if (vc->cell && *vc->cell) gc_mark_grey(*vc->cell);
    gc_trace_cont(vc->k);
}
static void varinit_cleanup(Cont *c) {
    VarInitCont *vc = (VarInitCont *)c;
    cont_release_abandon(vc->k);
}

/*
 * NK_ASSIGN: eval RHS, then call setter
 */
typedef struct {
    Cont         base;
    GraceObject *recv;
    char        *setter_name;
    Env         *env;
    Cont        *k;
} AssignRHSCont;

static PendingStep *assign_rhs_apply(Cont *c, GraceObject *val) {
    AssignRHSCont *ac = (AssignRHSCont *)c;
    GraceObject *recv = ac->recv;
    Env *env = env_retain(ac->env);
    char *setter = ac->setter_name;
    Cont *k = cont_retain(ac->k);
    ac->setter_name = NULL;
    cont_consumed(c);
    GraceObject *args[1] = { val };
    PendingStep *r = grace_request(recv, env, setter, args, 1, k);
    env_release(env);
    cont_release(k);
    free(setter);
    return r;
}
static void assign_rhs_trace(Cont *c) {
    AssignRHSCont *ac = (AssignRHSCont *)c;
    gc_mark_grey(ac->recv);
    gc_trace_env(ac->env);
    gc_trace_cont(ac->k);
}
static void assign_rhs_cont_cleanup(Cont *c) {
    AssignRHSCont *ac = (AssignRHSCont *)c;
    env_release(ac->env);
    cont_release_abandon(ac->k);
    free(ac->setter_name);
}

/*
 * NK_IMPORT_STMT: load or find module
 */
extern ASTNode *lexer_ast;  /* defined in baked/lexer.c */
extern ASTNode *parser_ast; /* defined in baked/parser.c */

/* File-scope continuation for NK_INTERP_STR start expression */
typedef struct { Cont base; InterpAsStrCont *ic; } InterpStartCont;
static PendingStep *interp_start_apply(Cont *c, GraceObject *v) {
    InterpStartCont *is_c = (InterpStartCont *)c;
    InterpAsStrCont *ic = is_c->ic;
    cont_retain((Cont *)ic);
    cont_consumed(c);
    PendingStep *r = grace_request(v, ic->env, "asString(0)", NULL, 0, (Cont *)ic);
    cont_release((Cont *)ic);
    return r;
}
static void interp_start_trace(Cont *c) {
    InterpStartCont *is_c = (InterpStartCont *)c;
    gc_trace_cont((Cont *)is_c->ic);
}
static void interp_start_cont_cleanup(Cont *c) {
    InterpStartCont *is_c = (InterpStartCont *)c;
    cont_release((Cont *)is_c->ic);
}

/* File-scope continuation for NK_RETURN_STMT */
typedef struct { Cont base; Cont *ret; } RetCont;
static PendingStep *ret_apply(Cont *c, GraceObject *v) {
    RetCont *rc = (RetCont *)c;
    Cont *ret = cont_retain(rc->ret);
    cont_consumed(c);
    PendingStep *r = cont_apply(ret, v);
    cont_release(ret);
    return r;
}
static void ret_trace(Cont *c) {
    RetCont *r = (RetCont *)c;
    gc_trace_cont(r->ret);
}
static void ret_cont_cleanup(Cont *c) {
    RetCont *rc = (RetCont *)c;
    cont_release_abandon(rc->ret);
}

/* File-scope continuation for dot-assignment LHS receiver */
typedef struct {
    Cont     base;
    ASTNode *rhs;
    char    *setter;
    Env     *env;
    Cont    *k;
} DotAssignRecvCont;
static PendingStep *da_recv_apply(Cont *c, GraceObject *recv2) {
    DotAssignRecvCont *d = (DotAssignRecvCont *)c;
    ASTNode *rhs = d->rhs;
    char *setter = d->setter;
    Env *env = env_retain(d->env);
    Cont *k = cont_retain(d->k);
    d->setter = NULL;
    cont_consumed(c);
    AssignRHSCont *ac = CONT_ALLOC(AssignRHSCont);
    ac->base.apply  = assign_rhs_apply;
    ac->base.gc_trace = assign_rhs_trace;
    ac->base.cleanup = assign_rhs_cont_cleanup;
    ac->recv        = recv2;
    ac->setter_name = setter;
    ac->env         = env_retain(env);
    ac->k           = cont_retain(k);
    PendingStep *r = eval_node(rhs, env, (Cont *)ac);
    env_release(env);
    cont_release(k);
    return r;
}
static void da_recv_trace(Cont *c) {
    DotAssignRecvCont *d = (DotAssignRecvCont *)c;
    gc_trace_env(d->env);
    gc_trace_cont(d->k);
}
static void da_recv_cont_cleanup(Cont *c) {
    DotAssignRecvCont *d = (DotAssignRecvCont *)c;
    env_release(d->env);
    cont_release_abandon(d->k);
    free(d->setter);
}

/*
 * eval_node - main evaluator dispatch
 */
PendingStep *eval_node(ASTNode *node, Env *env, Cont *k) {
    if (node == NULL)
        return cont_apply(k, grace_done);

    switch (node->kind) {

    /*  Literals  */
    case NK_NUMLIT:
        return cont_apply(k, grace_number_new(node->numval));

    case NK_STRLIT:
        return cont_apply(k, grace_string_new(node->strval ? node->strval : ""));

    /*  Interp string: strval=prefix, a1=expr, a2=next_interp  */
    case NK_INTERP_STR: {
        const char *prefix = node->strval ? node->strval : "";
        ASTNode *expr      = node->a1;
        ASTNode *next      = node->a2;
        if (expr == NULL)
            return cont_apply(k, grace_string_new(prefix));
        InterpAsStrCont *ic = CONT_ALLOC(InterpAsStrCont);
        ic->base.apply  = interp_asstr_cont_apply;
        ic->base.gc_trace = interp_asstr_cont_trace;
        ic->base.cleanup = interp_asstr_cont_cleanup;
        ic->prefix      = prefix;
        ic->next_interp = next;
        ic->env         = env_retain(env);
        ic->outer_k     = cont_retain(k);
        ic->accum       = str_dup(prefix);
        InterpStartCont *ec = CONT_ALLOC(InterpStartCont);
        ec->base.apply = interp_start_apply;
        ec->base.gc_trace = interp_start_trace;
        ec->base.cleanup = interp_start_cont_cleanup;
        ec->ic = ic;
        cont_retain((Cont *)ic);
        return eval_node(expr, env, (Cont *)ec);
    }

    /*  Block  */
    case NK_BLOCK: {
        GraceObject *blk = grace_block_new(
            node->a1,  /* params */
            node->a2,  /* body */
            env->scope,
            env->receiver,
            env->return_k,
            env->except_k);
        return cont_apply(k, blk);
    }

    /*  Lineup  */
    case NK_LINEUP: {
        ASTNode *elems = node->a1;  /* cons list of element ASTNodes */
        int n = list_length(elems);
        if (n == 0)
            return cont_apply(k, grace_lineup_new(NULL, 0));
        GraceObject **arr = malloc((size_t)n * sizeof(GraceObject *));
        LineupBuildCont *lb = CONT_ALLOC(LineupBuildCont);
        lb->base.apply    = lineup_build_apply;
        lb->base.gc_trace = lineup_build_trace;
        lb->base.cleanup  = lineup_build_cleanup;
        lb->rem   = elems;
        lb->arr   = arr;
        lb->idx   = 0;
        lb->total = n;
        lb->env   = env_retain(env);
        lb->k     = cont_retain(k);
        return eval_node(elems->a1, env, (Cont *)lb);
    }

    /*  lexical request  */
    case NK_LEXREQ: {
        const char *name = node->strval;
        ASTNode    *args_n = node->a1;
        /* Find receiver for this name */
        GraceObject *recv = scope_find_receiver(env->scope, name);
        if (!recv) recv = scope_find_receiver((GraceObject *)env->receiver, name);
        if (!recv) recv = env->receiver;  /* fallback: self */
        int n = list_length(args_n);
        if (n == 0)
            return grace_request(recv, env, name, NULL, 0, k);
        /* Build flat arg list from parts (already flat for lexreq) */
        DotRecvCont *dc = CONT_ALLOC(DotRecvCont);
        dc->base.apply = dot_recv_cont_apply;
        dc->base.gc_trace = dot_recv_cont_trace;
        dc->base.cleanup = dot_recv_cont_cleanup;
        dc->flat_args  = args_n;
        dc->env        = env_retain(env);
        dc->name       = str_dup(name);
        dc->k          = cont_retain(k);
        return dot_recv_cont_apply((Cont *)dc, recv);
    }

    /*  dot/explicit request  */
    case NK_DOTREQ: {
        ASTNode    *recv_node = node->a1;
        const char *name      = node->strval;
        ASTNode    *args_n    = node->a2;
        DotRecvCont *dc = CONT_ALLOC(DotRecvCont);
        dc->base.apply = dot_recv_cont_apply;
        dc->base.gc_trace = dot_recv_cont_trace;
        dc->base.cleanup = dot_recv_cont_cleanup;
        dc->flat_args  = args_n;
        dc->env        = env_retain(env);
        dc->name       = str_dup(name);
        dc->k          = cont_retain(k);
        return eval_node(recv_node, env, (Cont *)dc);
    }

    /*  Object constructor  */
    case NK_OBJCONS: {
        GraceUserObject *obj = (GraceUserObject *)grace_user_new(env->scope);
        /* Inherit dialect from outer scope */
        GraceObject *outer = env->scope;
        if (outer && outer->vt == &grace_user_vtable)
            obj->dialect = ((GraceUserObject *)outer)->dialect;
        Env *inner = malloc(sizeof(Env));
        inner->refcount = 1;
        inner->receiver = (GraceObject *)obj;
        inner->scope    = (GraceObject *)obj;
        inner->return_k = cont_retain(cont_done);      /* return inside object body is unusual */
        inner->except_k = cont_retain(env->except_k);
        inner->reset_k  = cont_retain(env->reset_k);
        /* Bind self(0) on the object so blocks in the constructor body
         * can refer to `self` and get this object, not an outer scope's self. */
        user_bind_def((GraceObject *)obj, "self", (GraceObject *)obj);
        /* Pre-pass: hoist all method declarations so they are visible
         * to var/def initializers in the same body (Grace semantics).
         * Also hoist def declarations as placeholders (so `use` won't
         * override locally-defined names), and register inherit variants
         * for methods whose body ends with an object constructor. */
        for (ASTNode *n = node->a1; n; ) {
            ASTNode *stmt;
            if (n->kind == NK_CONS) { stmt = n->a1; n = n->a2; }
            else                    { stmt = n;     n = NULL;  }
            if (!stmt) continue;
            if (stmt->kind == NK_METH_DECL) {
                char *mname = parts_to_name(stmt->a1);
                MethodClosure *mc = malloc(sizeof(MethodClosure));
                mc->parts     = stmt->a1;
                mc->body      = stmt->a4;
                mc->lex_scope = (GraceObject *)obj;
                mc->lex_self  = (GraceObject *)obj;
                user_add_method((GraceObject *)obj, mname, method_fn, mc);
                set_last_method_gc((GraceObject *)obj, method_closure_trace, method_closure_free);
                if (body_tail_is_objcons(stmt->a4)) {
                    char *iname = str_fmt("%sinherit(1)", mname);
                    MethodClosure *imc = malloc(sizeof(MethodClosure));
                    imc->parts     = stmt->a1;
                    imc->body      = stmt->a4;
                    imc->lex_scope = (GraceObject *)obj;
                    imc->lex_self  = (GraceObject *)obj;
                    user_add_method((GraceObject *)obj, iname, method_inherit_fn, imc);
                    set_last_method_gc((GraceObject *)obj, method_closure_trace, method_closure_free);
                    free(iname);
                }
                free(mname);
            } else if (stmt->kind == NK_DEF_DECL) {
                user_bind_def((GraceObject *)obj, stmt->strval, grace_uninit);
            }
        }
        ObjDoneCont *oc = CONT_ALLOC(ObjDoneCont);
        oc->base.apply  = objdone_apply;
        oc->base.gc_trace = objdone_trace;
        oc->base.cleanup = objdone_cleanup;
        oc->obj = (GraceObject *)obj;
        oc->k   = cont_retain(k);
        {
            PendingStep *r = eval_stmts(node->a1, inner, (Cont *)oc);
            env_release(inner);
            return r;
        }
    }

    /*  def declaration  */
    case NK_DEF_DECL: {
        /* node->strval = name, node->a3 = value */
        const char *name  = node->strval;
        ASTNode    *value = node->a3;
        DefBindCont *dc = CONT_ALLOC(DefBindCont);
        dc->base.apply = defbind_apply;
        dc->base.gc_trace = defbind_trace;
        dc->base.cleanup = defbind_cleanup;
        dc->name   = name;
        dc->target = env->scope;   /* bind on scope (frame for methods, obj for object bodies) */
        dc->k      = cont_retain(k);
        if (value)
            return eval_node(value, env, (Cont *)dc);
        user_bind_def(env->scope, name, grace_done);
        return cont_apply(k, grace_done);
    }

    /*  var declaration  */
    case NK_VAR_DECL: {
        const char *name  = node->strval;
        ASTNode    *init  = node->a3;
        GraceObject **cell = malloc(sizeof(GraceObject *));
        *cell = grace_uninit;
        user_bind_var(env->scope, name, cell);  /* bind on scope (frame for methods) */
        if (init) {
            VarInitCont *vc = CONT_ALLOC(VarInitCont);
            vc->base.apply = varinit_apply;
            vc->base.gc_trace = varinit_trace;
            vc->base.cleanup = varinit_cleanup;
            vc->cell = cell;
            vc->k    = cont_retain(k);
            return eval_node(init, env, (Cont *)vc);
        }
        return cont_apply(k, grace_done);
    }

    /*  method declaration  */
    case NK_METH_DECL: {
        /* node->a1 = parts list, node->a4 = body */
        char *mname = parts_to_name(node->a1);
        MethodClosure *mc = malloc(sizeof(MethodClosure));
        mc->parts     = node->a1;
        mc->body      = node->a4;
        mc->lex_scope = env->scope;
        mc->lex_self  = env->receiver;
        user_add_method(env->scope, mname, method_fn, mc);  /* bind on scope */
        set_last_method_gc(env->scope, method_closure_trace, method_closure_free);
        if (body_tail_is_objcons(node->a4)) {
            char *iname = str_fmt("%sinherit(1)", mname);
            MethodClosure *imc = malloc(sizeof(MethodClosure));
            imc->parts     = node->a1;
            imc->body      = node->a4;
            imc->lex_scope = env->scope;
            imc->lex_self  = env->receiver;
            user_add_method(env->scope, iname, method_inherit_fn, imc);
            set_last_method_gc(env->scope, method_closure_trace, method_closure_free);
            free(iname);
        }
        free(mname);
        return cont_apply(k, grace_done);
    }

    /*  return statement  */
    case NK_RETURN_STMT:
        if (node->a1) {
            RetCont *rc = CONT_ALLOC(RetCont);
            rc->base.apply = ret_apply;
            rc->base.gc_trace = ret_trace;
            rc->base.cleanup = ret_cont_cleanup;
            rc->ret = cont_retain(env->return_k);
            return eval_node(node->a1, env, (Cont *)rc);
        }
        return cont_apply(env->return_k, grace_done);

    /*  assignment  */
    case NK_ASSIGN: {
        ASTNode *lhs = node->a1;
        ASTNode *rhs = node->a2;
        char *setter;
        GraceObject *recv;
        if (lhs->kind == NK_LEXREQ) {
            setter = getter_to_setter(lhs->strval);
            recv = scope_find_receiver(env->scope, setter);
            if (!recv) recv = env->receiver;
        } else if (lhs->kind == NK_DOTREQ) {
            setter = getter_to_setter(lhs->strval);
            recv = NULL;  /* will be evaluated */
        } else {
            grace_fatal("Assignment LHS must be identifier or dot request");
        }
        if (lhs->kind == NK_LEXREQ) {
            AssignRHSCont *ac = CONT_ALLOC(AssignRHSCont);
            ac->base.apply = assign_rhs_apply;
            ac->base.gc_trace = assign_rhs_trace;
            ac->base.cleanup = assign_rhs_cont_cleanup;
            ac->recv        = recv;
            ac->setter_name = setter;
            ac->env         = env_retain(env);
            ac->k           = cont_retain(k);
            return eval_node(rhs, env, (Cont *)ac);
        } else {
            /* Dot assign: eval receiver, then rhs, then call setter */
            DotAssignRecvCont *dr = CONT_ALLOC(DotAssignRecvCont);
            dr->base.apply = da_recv_apply;
            dr->base.gc_trace = da_recv_trace;
            dr->base.cleanup = da_recv_cont_cleanup;
            dr->rhs    = rhs;
            dr->setter = setter;
            dr->env    = env_retain(env);
            dr->k      = cont_retain(k);
            return eval_node(lhs->a1, env, (Cont *)dr);
        }
    }

    /*  dialect statement  */
    case NK_DIALECT_STMT: {
        const char *src = node->strval;
        GraceObject *mod = grace_find_module(src);
        if (!mod) grace_fatal("Cannot find dialect '%s'", src);
        if (env->scope->vt == &grace_user_vtable) {
            GraceUserObject *uo = (GraceUserObject *)env->scope;
            uo->dialect = mod;
        }
        return cont_apply(k, grace_done);
    }

    /*  import statement  */
    case NK_IMPORT_STMT: {
        const char *src     = node->strval;
        ASTNode    *binding = node->a1;   /* NK_IDENT_DECL */
        GraceObject *mod = grace_find_module(src);
        if (!mod) {
            /* Import loading runs nested synchronous trampolines. Keep the
             * current object/scope rooted so the enclosing evaluation context
             * cannot be swept while the module is being parsed and evaluated.
             * Also protect the outer continuation k from gc_sweep_conts. */
            gc_push_root(&env->receiver);
            gc_push_root(&env->scope);
            gc_push_cont_root(&k);
            /* Load src.grace from disk, parse, eval, and register */
            char path[512];
            snprintf(path, sizeof(path), "%s.grace", src);
            FILE *mf = fopen(path, "r");
            if (!mf)
                grace_fatal("Cannot find module '%s': no such file '%s'", src, path);
            fseek(mf, 0, SEEK_END);
            long msz = ftell(mf);
            fseek(mf, 0, SEEK_SET);
            char *msrc = malloc(msz + 1);
            fread(msrc, 1, msz, mf);
            msrc[msz] = '\0';
            fclose(mf);
            GraceObject *parser_obj = grace_find_module("//parser");
            if (!parser_obj)
                grace_fatal("Parser not available for loading module '%s'", src);
            GraceObject *pargs[2] = {
                grace_string_new(path),
                grace_string_new(msrc)   /* msrc ownership transferred */
            };
            GraceObject *ast_obj = grace_request_sync(parser_obj, env,
                                                       "parseModule(2)", pargs, 2);
            ASTNode *prog = grace_ast_to_astnode(ast_obj, env);
            if (!prog)
                grace_fatal("Parser returned nothing for module '%s'", src);
            CaptureCont *mcc = CONT_ALLOC(CaptureCont);
            mcc->base.apply = capture_apply;
            mcc->base.gc_trace = capture_cont_trace;
            mcc->base.cleanup = NULL;
            mcc->result = grace_done;
            cont_retain((Cont *)mcc);
            trampoline(eval_node(prog, env, (Cont *)mcc));
            mod = mcc->result;
            cont_release((Cont *)mcc);
            grace_register_module(str_dup(src), mod);
            gc_pop_cont_root();
            gc_pop_roots(2);
        }
        const char *bname = binding ? binding->strval : src;
        user_bind_def(env->receiver, bname, mod);
        user_bind_def(env->scope,    bname, mod);
        return cont_apply(k, grace_done);
    }

    /*  inherit statement  */
    case NK_INHERIT: {
        ASTNode *parent_expr = node->a1;
        if (!parent_expr || parent_expr->kind != NK_LEXREQ)
            return grace_raise(env, "InheritError",
                "inherit expression must be a simple request");
        const char *orig_name = parent_expr->strval;
        char *inherit_name = str_fmt("%sinherit(1)", orig_name);
        GraceObject *recv = scope_find_receiver(env->scope, inherit_name);
        if (!recv) recv = scope_find_receiver(env->receiver, inherit_name);
        if (!recv) {
            free(inherit_name);
            return grace_raise(env, "InheritError",
                "Cannot inherit from '%s': not a fresh object constructor",
                orig_name);
        }
        GraceObject *inheriting = env->receiver;
        GraceObject *iargs[1] = { inheriting };
        PendingStep *r = grace_request(recv, env, inherit_name, iargs, 1, k);
        free(inherit_name);
        return r;
    }

    /*  use statement  */
    case NK_USE: {
        ASTNode *source_expr = node->a1;
        UseSourceCont *usc = CONT_ALLOC(UseSourceCont);
        usc->base.apply    = use_source_apply;
        usc->base.gc_trace = use_source_trace;
        usc->base.cleanup  = use_source_cleanup;
        usc->env = env_retain(env);
        usc->k   = cont_retain(k);
        return eval_node(source_expr, env, (Cont *)usc);
    }

    /*  ignored/trivial nodes  */
    case NK_COMMENT:
    case NK_TYPE_DECL:
    case NK_IFACE_CONS:
    case NK_METH_SIG:
        return cont_apply(k, grace_done);

    /*  cons: should be handled by eval_stmts, not here  */
    case NK_CONS:
        return eval_stmts(node, env, k);

    default:
        grace_fatal("eval_node: unknown node kind %d", node->kind);
    }
    return NULL; /* unreachable */
}
