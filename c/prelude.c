/* prelude.c - The Grace standard prelude object */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grace.h"
#include "gc.h"

/*  while(1)do(1)  */

/* Trampoline-friendly loop state. Chain of continuations evaluating
 * condition block, if true the body block, then loop.
 *
 * State carried across each iteration:
 */
typedef struct WhileState {
    GraceObject *cond_blk;
    GraceObject *body_blk;
    Env         *env;
    Cont        *exit_k;   /* k when the loop finishes */
} WhileState;

typedef struct WhileBodyCont { Cont base; WhileState *ws; } WhileBodyCont;
typedef struct WhileCondCont { Cont base; WhileState *ws; } WhileCondCont;

static void while_trace(Cont *c) {
    /* Both WhileCondCont and WhileBodyCont have the same layout */
    WhileCondCont *wc = (WhileCondCont *)c;
    WhileState *ws = wc->ws;
    if (!ws) return;
    gc_mark_grey(ws->cond_blk);
    gc_mark_grey(ws->body_blk);
    gc_trace_env(ws->env);
    gc_trace_cont(ws->exit_k);
}
static void while_cleanup(Cont *c) {
    WhileCondCont *wc = (WhileCondCont *)c;
    WhileState *ws = wc->ws;
    if (!ws) return;
    env_release(ws->env);
    cont_release_abandon(ws->exit_k);
    free(ws);
}

static PendingStep *while_cond_apply(Cont *c, GraceObject *cond_result);
static PendingStep *while_body_apply(Cont *c, GraceObject *body_result) {
    (void)body_result;
    WhileBodyCont *wb = (WhileBodyCont *)c;
    WhileState *ws = wb->ws;
    /* re-evaluate condition */
    WhileCondCont *wc = CONT_ALLOC(WhileCondCont);
    wc->base.apply = while_cond_apply;
    wc->base.gc_trace = while_trace;
    wc->base.cleanup = while_cleanup;
    wc->ws = ws;
    wb->ws = NULL;
    cont_consumed(c);
    return grace_request(ws->cond_blk, ws->env, "apply(0)", NULL, 0, (Cont *)wc);
}

static PendingStep *while_cond_apply(Cont *c, GraceObject *cond_result) {
    WhileCondCont *wc = (WhileCondCont *)c;
    WhileState *ws = wc->ws;
    if (cond_result->vt != &grace_bool_vtable || !grace_bool_val(cond_result)) {
        Cont *exit_k = cont_retain(ws->exit_k);
        wc->ws = NULL;
        cont_consumed(c);
        env_release(ws->env);
        cont_release(ws->exit_k);
        free(ws);
        PendingStep *r = cont_apply(exit_k, grace_done);
        cont_release(exit_k);
        return r;
    }
    WhileBodyCont *wb = CONT_ALLOC(WhileBodyCont);
    wb->base.apply = while_body_apply;
    wb->base.gc_trace = while_trace;
    wb->base.cleanup = while_cleanup;
    wb->ws = ws;
    wc->ws = NULL;
    cont_consumed(c);
    return grace_request(ws->body_blk, ws->env, "apply(0)", NULL, 0, (Cont *)wb);
}

/*  print(1)  */
typedef struct { Cont base; Env *env; Cont *k; } PrintAsStrCont;

static void print_asstr_trace(Cont *c) {
    PrintAsStrCont *pc = (PrintAsStrCont *)c;
    gc_trace_env(pc->env);
    gc_trace_cont(pc->k);
}
static void print_asstr_cleanup(Cont *c) {
    PrintAsStrCont *pc = (PrintAsStrCont *)c;
    env_release(pc->env);
    cont_release_abandon(pc->k);
}
static PendingStep *print_asstr_apply(Cont *c, GraceObject *str_val) {
    PrintAsStrCont *pc = (PrintAsStrCont *)c;
    const char *s = grace_string_val(str_val);
    Env *env = env_retain(pc->env);
    Cont *k = cont_retain(pc->k);
    cont_consumed(c);
    printf("%s\n", s);
    env_release(env);
    PendingStep *r = cont_apply(k, grace_done);
    cont_release(k);
    return r;
}

static PendingStep *prelude_print_fn(GraceObject *self, Env *env,
                                      GraceObject **args, int nargs, Cont *k,
                                      void *data) {
    (void)self; (void)nargs; (void)data;
    PrintAsStrCont *pc = CONT_ALLOC(PrintAsStrCont);
    pc->base.apply = print_asstr_apply;
    pc->base.gc_trace = print_asstr_trace;
    pc->base.cleanup = print_asstr_cleanup;
    pc->env = env_retain(env);
    pc->k   = cont_retain(k);
    return grace_request(args[0], env, "asString(0)", NULL, 0, (Cont *)pc);
}

/*  try(1)catch(1)  */
/*
 * try { body } catch { exc -> ... }
 * args[0] = body block, args[1] = catch block (takes one arg: the exception)
 */
typedef struct TryCatchData {
    Cont         base;
    GraceObject *catch_blk;
    Env         *env;
    Cont        *k;
} TryCatchData;

static void try_catch_trace(Cont *c) {
    TryCatchData *td = (TryCatchData *)c;
    gc_mark_grey(td->catch_blk);
    gc_trace_env(td->env);
    gc_trace_cont(td->k);
}
static void try_catch_cleanup(Cont *c) {
    TryCatchData *td = (TryCatchData *)c;
    env_release(td->env);
    cont_release_abandon(td->k);
}
static PendingStep *try_catch_except_apply(Cont *c, GraceObject *ex) {
    TryCatchData *td = (TryCatchData *)c;
    GraceObject *catch_blk = td->catch_blk;
    Env *env = env_retain(td->env);
    Cont *k = cont_retain(td->k);
    cont_consumed(c);
    GraceObject *args[1] = { ex };
    PendingStep *r = grace_request(catch_blk, env, "apply(1)", args, 1, k);
    env_release(env);
    cont_release(k);
    return r;
}

static PendingStep *prelude_try_catch_fn(GraceObject *self, Env *env,
                                          GraceObject **args, int nargs,
                                          Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    GraceObject *body_blk  = args[0];
    GraceObject *catch_blk = args[1];
    TryCatchData *td = CONT_ALLOC(TryCatchData);
    td->base.apply  = try_catch_except_apply;
    td->base.gc_trace = try_catch_trace;
    td->base.cleanup = try_catch_cleanup;
    td->catch_blk   = catch_blk;
    td->env         = env_retain(env);
    td->k           = cont_retain(k);
    /* Override except_k in env */
    Env *try_env = malloc(sizeof(Env));
    *try_env = *env;
    try_env->refcount = 1;
    cont_retain(try_env->return_k);  /* copy inherited return_k ref */
    cont_retain(try_env->reset_k);   /* copy inherited reset_k ref */
    try_env->except_k = cont_retain((Cont *)td);
    PendingStep *r = grace_request(body_blk, try_env, "apply(0)", NULL, 0, k);
    env_release(try_env);
    return r;
}

/*  getFileContents(1)  */
static PendingStep *prelude_getFileContents_fn(GraceObject *self, Env *env,
                                                GraceObject **args, int nargs,
                                                Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    const char *path = grace_string_val(args[0]);
    FILE *f = fopen(path, "r");
    if (!f) grace_raise(env, "FileNotFound", "Cannot open file '%s'", path);
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return cont_apply(k, grace_string_new(buf));
}

/*  match case helpers  */
/*
 * match(1)case(1): match subject against one block-pattern
 * match(1)case(1)case(1): two cases, etc.
 */
typedef struct {
    GraceObject *subject;
    GraceObject **cases;
    int          ncases;
    Env         *env;
    Cont        *k;
} MatchState;

static void match_state_free(MatchState *ms) {
    if (!ms) return;
    free(ms->cases);
    env_release(ms->env);
    cont_release_abandon(ms->k);
    free(ms);
}

static PendingStep *match_try_case(MatchState *ms, int idx);

typedef struct { Cont base; MatchState *ms; int idx; } MatchCaseCont;

static void match_case_trace(Cont *c) {
    MatchCaseCont *mc = (MatchCaseCont *)c;
    MatchState *ms = mc->ms;
    if (!ms) return;
    gc_mark_grey(ms->subject);
    for (int i = 0; i < ms->ncases; i++)
        gc_mark_grey(ms->cases[i]);
    gc_trace_env(ms->env);
    gc_trace_cont(ms->k);
}
static void match_case_cleanup(Cont *c) {
    MatchCaseCont *mc = (MatchCaseCont *)c;
    match_state_free(mc->ms);
}

static PendingStep *match_case_apply(Cont *c, GraceObject *result) {
    MatchCaseCont *mc = (MatchCaseCont *)c;
    MatchState *ms = mc->ms;
    int idx = mc->idx;
    mc->ms = NULL;
    cont_consumed(c);
    /* result is the match result object: .succeeded, .result
     * Protect conts from gc_sweep_conts during sync requests. */
    gc_push_cont_root(&ms->k);
    gc_push_cont_root(&ms->env->return_k);
    gc_push_cont_root(&ms->env->except_k);
    GraceObject *succ = grace_request_sync(result, ms->env, "succeeded(0)", NULL, 0);
    if (succ->vt == &grace_bool_vtable && grace_bool_val(succ)) {
        GraceObject *val = grace_request_sync(result, ms->env, "result(0)", NULL, 0);
        gc_pop_cont_root();
        gc_pop_cont_root();
        gc_pop_cont_root();
        Cont *k = cont_retain(ms->k);
        match_state_free(ms);
        PendingStep *r = cont_apply(k, val);
        cont_release(k);
        return r;
    }
    gc_pop_cont_root();
    gc_pop_cont_root();
    gc_pop_cont_root();
    int next = idx + 1;
    if (next >= ms->ncases) {
        Cont *k = cont_retain(ms->k);
        match_state_free(ms);
        PendingStep *r = cont_apply(k, grace_done);   /* no match */
        cont_release(k);
        return r;
    }
    return match_try_case(ms, next);
}

static PendingStep *match_try_case(MatchState *ms, int idx) {
    GraceObject *cas = ms->cases[idx];
    MatchCaseCont *mc = CONT_ALLOC(MatchCaseCont);
    mc->base.apply = match_case_apply;
    mc->base.gc_trace = match_case_trace;
    mc->base.cleanup = match_case_cleanup;
    mc->ms  = ms;
    mc->idx = idx;
    GraceObject *args[1] = { ms->subject };
    return grace_request(cas, ms->env, "match(1)", args, 1, (Cont *)mc);
}

static PendingStep *build_match_fn(GraceObject *self, Env *env,
                                    GraceObject **args, int nargs, Cont *k,
                                    void *data) {
    (void)self; (void)data;
    /* args[0] = subject, args[1..nargs-1] = case blocks */
    GraceObject *subject = args[0];
    int ncases = nargs - 1;
    MatchState *ms = malloc(sizeof(MatchState));
    ms->subject = subject;
    ms->cases   = malloc(ncases * sizeof(GraceObject *));
    for (int i = 0; i < ncases; i++) ms->cases[i] = args[1 + i];
    ms->ncases  = ncases;
    ms->env     = env_retain(env);
    ms->k       = cont_retain(k);
    return match_try_case(ms, 0);
}

/*  while(1)do(1) method fn  */
static PendingStep *prelude_while_do_fn(GraceObject *self, Env *env,
                                         GraceObject **args, int nargs,
                                         Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    GraceObject *cond_blk = args[0];
    GraceObject *body_blk = args[1];
    WhileState *ws = malloc(sizeof(WhileState));
    ws->cond_blk = cond_blk;
    ws->body_blk = body_blk;
    ws->env      = env_retain(env);
    ws->exit_k   = cont_retain(k);
    WhileCondCont *wc = CONT_ALLOC(WhileCondCont);
    wc->base.apply = while_cond_apply;
    wc->base.gc_trace = while_trace;
    wc->base.cleanup = while_cleanup;
    wc->ws = ws;
    return grace_request(cond_blk, env, "apply(0)", NULL, 0, (Cont *)wc);
}

/*  for(1)do(1) method fn  */
typedef struct { Cont base; GraceObject *blk; Env *env; Cont *k; } ForIterCont;

static PendingStep *prelude_for_do_fn(GraceObject *self, Env *env,
                                       GraceObject **args, int nargs,
                                       Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    GraceObject *iter = args[0];  /* range or list with do(1) */
    GraceObject *blk  = args[1];
    GraceObject *do_args[1] = { blk };
    return grace_request(iter, env, "do(1)", do_args, 1, k);
}

/*  if(1)then(1)else(1) variants  */
typedef struct { Cont base; GraceObject *then_blk; GraceObject *else_blk; Env *env; Cont *k; } IfCont;

// static PendingStep *if_cond_apply(Cont *c, GraceObject *cond) {
//     IfCont *ic = (IfCont *)c;
//     int t = (cond->vt == &grace_bool_vtable) && grace_bool_val(cond);
//     GraceObject *blk = t ? ic->then_blk : ic->else_blk;
//     if (!blk) return cont_apply(ic->k, grace_done);
//     return grace_request(blk, ic->env, "apply(0)", NULL, 0, ic->k);
// }

// static IfCont *make_if_cont(GraceObject *then_blk, GraceObject *else_blk,
//                              Env *env, Cont *k) {
//     IfCont *ic = malloc(sizeof(IfCont));
//     ic->base.apply = if_cond_apply;
//     ic->then_blk = then_blk;
//     ic->else_blk = else_blk;
//     ic->env = env;
//     ic->k   = k;
//     return ic;
// }

static PendingStep *prelude_if_then_fn(GraceObject *self, Env *env,
                                        GraceObject **args, int nargs,
                                        Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    /* args[0] = boolean condition (not a block), args[1] = then block */
    int t = (args[0]->vt == &grace_bool_vtable) && grace_bool_val(args[0]);
    if (!t) return cont_apply(k, grace_done);
    return grace_request(args[1], env, "apply(0)", NULL, 0, k);
}

static PendingStep *prelude_if_then_else_fn(GraceObject *self, Env *env,
                                             GraceObject **args, int nargs,
                                             Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    /* args[0] = boolean condition, args[1] = then block, args[2] = else block */
    int t = (args[0]->vt == &grace_bool_vtable) && grace_bool_val(args[0]);
    GraceObject *blk = t ? args[1] : args[2];
    return grace_request(blk, env, "apply(0)", NULL, 0, k);
}

/*  CPS elseif chain  */
/* Elseif condition is a block, so need to CPS-apply it and then
 * check the result.  This continuation handles the check and recurses.
 *
 * Invariant: saved_args[cur_idx] is the condition block just applied;
 *            saved_args[cur_idx+1] is the corresponding then-block.
 * has_else: 1 if saved_args[nargs-1] is a final else block (nargs is odd).
 */
#define MAX_ELSEIF_ARGS 16
typedef struct {
    Cont          base;
    int           nargs;
    int           cur_idx;
    int           has_else;
    GraceObject  *saved_args[MAX_ELSEIF_ARGS];
    Env          *env;
    Cont         *outer_k;
} ElseifCont;

static void elseif_trace(Cont *c) {
    ElseifCont *ec = (ElseifCont *)c;
    for (int i = 0; i < ec->nargs && i < MAX_ELSEIF_ARGS; i++)
        gc_mark_grey(ec->saved_args[i]);
    gc_trace_env(ec->env);
    gc_trace_cont(ec->outer_k);
}
static void elseif_cleanup(Cont *c) {
    ElseifCont *ec = (ElseifCont *)c;
    env_release(ec->env);
    cont_release_abandon(ec->outer_k);
}

static PendingStep *elseif_cont_fn(Cont *ck, GraceObject *cond_result) {
    ElseifCont *ec = (ElseifCont *)ck;
    int t = (cond_result->vt == &grace_bool_vtable) && grace_bool_val(cond_result);
    if (t) {
        /* Condition was true: apply the corresponding then-block */
        GraceObject *blk = ec->saved_args[ec->cur_idx + 1];
        Env *env = env_retain(ec->env);
        Cont *outer_k = cont_retain(ec->outer_k);
        cont_consumed(ck);
        PendingStep *r = grace_request(blk, env, "apply(0)", NULL, 0, outer_k);
        env_release(env);
        cont_release(outer_k);
        return r;
    }
    /* Condition was false: try the next elseif pair */
    int next = ec->cur_idx + 2;
    int pairs_end = ec->has_else ? ec->nargs - 1 : ec->nargs;
    if (next + 1 < pairs_end) {
        /* Another elseif condition to evaluate - apply it */
        ElseifCont *nc = CONT_ALLOC(ElseifCont);
        *nc = *ec;
        nc->base.refcount = 1;  /* reset after struct copy */
        nc->base.consumed = 0;
        nc->cur_idx = next;
        nc->env = env_retain(ec->env);  /* nc gets its own ref */
        cont_retain(nc->outer_k);       /* nc gets its own ref to outer_k */
        GraceObject *blk = ec->saved_args[next];
        Env *env = env_retain(ec->env);
        cont_consumed(ck);
        PendingStep *r = grace_request(blk, env, "apply(0)", NULL, 0, (Cont *)nc);
        env_release(env);
        return r;
    }
    if (next < pairs_end) {
        ElseifCont *nc = CONT_ALLOC(ElseifCont);
        *nc = *ec;
        nc->base.refcount = 1;  /* reset after struct copy */
        nc->base.consumed = 0;
        nc->cur_idx = next;
        nc->env = env_retain(ec->env);
        cont_retain(nc->outer_k);
        GraceObject *blk = ec->saved_args[next];
        Env *env = env_retain(ec->env);
        cont_consumed(ck);
        PendingStep *r = grace_request(blk, env, "apply(0)", NULL, 0, (Cont *)nc);
        env_release(env);
        return r;
    }
    /* No more elseif conditions */
    if (ec->has_else) {
        GraceObject *blk = ec->saved_args[ec->nargs - 1];
        Env *env = env_retain(ec->env);
        Cont *outer_k = cont_retain(ec->outer_k);
        cont_consumed(ck);
        PendingStep *r = grace_request(blk, env, "apply(0)", NULL, 0, outer_k);
        env_release(env);
        cont_release(outer_k);
        return r;
    }
    Cont *outer_k = cont_retain(ec->outer_k);
    cont_consumed(ck);
    PendingStep *r = cont_apply(outer_k, grace_done);
    cont_release(outer_k);
    return r;
}

/* Helper: start checking elseif conditions from args[start_idx].
 * args[0] has already been checked (direct bool) and was false. */
static PendingStep *start_elseif_chain(GraceObject **args, int nargs,
                                        int start_idx, int has_else,
                                        Env *env, Cont *k) {
    int pairs_end = has_else ? nargs - 1 : nargs;
    if (start_idx >= pairs_end) {
        /* No elseif conditions - jump straight to else or done */
        if (has_else)
            return grace_request(args[nargs - 1], env, "apply(0)", NULL, 0, k);
        return cont_apply(k, grace_done);
    }
    ElseifCont *ec = CONT_ALLOC(ElseifCont);
    ec->base.apply    = elseif_cont_fn;
    ec->base.gc_trace = elseif_trace;
    ec->base.cleanup = elseif_cleanup;
    ec->nargs    = nargs;
    ec->cur_idx  = start_idx;
    ec->has_else = has_else;
    ec->env      = env_retain(env);
    ec->outer_k  = cont_retain(k);
    int n = nargs < MAX_ELSEIF_ARGS ? nargs : MAX_ELSEIF_ARGS;
    for (int i = 0; i < n; i++) ec->saved_args[i] = args[i];
    /* Apply the first elseif condition block (lazy) */
    return grace_request(args[start_idx], env, "apply(0)", NULL, 0, (Cont *)ec);
}

/* Generic if(1)then(1)[elseif(1)then(1)]...else(1) handler.
 * args[0] = direct-bool condition (already evaluated by the caller),
 * args[1] = then block,
 * args[2,4,...] = elseif condition blocks (lazy, need apply(0)),
 * args[3,5,...] = corresponding then blocks,
 * args[nargs-1] = else block  (nargs is odd).
 */
static PendingStep *prelude_if_elseif_else_fn(GraceObject *self, Env *env,
                                               GraceObject **args, int nargs,
                                               Cont *k, void *data) {
    (void)self; (void)data;
    /* Check the initial if-condition (direct bool) */
    int t = (args[0]->vt == &grace_bool_vtable) && grace_bool_val(args[0]);
    if (t)
        return grace_request(args[1], env, "apply(0)", NULL, 0, k);
    /* Fall through to elseif chain; last arg is the else block */
    return start_elseif_chain(args, nargs, 2, 1, env, k);
}

/* Generic if(1)then(1)[elseif(1)then(1)]... handler. (no final else)
 * Like above but nargs is even and there is no else block.
 */
static PendingStep *prelude_if_elseif_fn(GraceObject *self, Env *env,
                                          GraceObject **args, int nargs,
                                          Cont *k, void *data) {
    (void)self; (void)data;
    int t = (args[0]->vt == &grace_bool_vtable) && grace_bool_val(args[0]);
    if (t)
        return grace_request(args[1], env, "apply(0)", NULL, 0, k);
    /* Fall through to elseif chain; no else block */
    return start_elseif_chain(args, nargs, 2, 0, env, k);
}

/*
 * make_prelude - builds and returns the prelude object
 */
static PendingStep *true_fn(GraceObject *s, Env *e, GraceObject **a,
                             int n, Cont *k, void *d) {
    (void)s;(void)e;(void)a;(void)n;(void)d;
    return cont_apply(k, grace_true);
}
static PendingStep *false_fn(GraceObject *s, Env *e, GraceObject **a,
                              int n, Cont *k, void *d) {
    (void)s;(void)e;(void)a;(void)n;(void)d;
    return cont_apply(k, grace_false);
}

static PendingStep *prelude_number_fn(GraceObject *s, Env *e, GraceObject **a,
                                       int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    /* Number(x) - if given a string, convert; if given a number, return it */
    if (a[0]->vt == &grace_number_vtable) return cont_apply(k, a[0]);
    const char *sv = grace_string_val(a[0]);
    char *ep; double num = strtod(sv, &ep);
    if (ep == sv) grace_raise(e, "TypeError", "Not a number: '%s'", sv);
    return cont_apply(k, grace_number_new(num));
}

static PendingStep *prelude_string_fn(GraceObject *s, Env *e, GraceObject **a,
                                       int n, Cont *k, void *d) {
    (void)s;(void)e;(void)n;(void)d;
    return grace_request(a[0], e, "asString(0)", NULL, 0, k);
}

static PendingStep *prelude_exception_fn(GraceObject *s, Env *e, GraceObject **a,
                                          int n, Cont *k, void *d) {
    (void)s;(void)e;(void)a;(void)n;(void)d;
    return cont_apply(k, grace_exception_proto_new("Exception"));
}

/*
 * Delimited continuations: reset(1) and shift(1)
 */

/*
 * PromptBox is a mutable cell holding the "outer_k" of the current reset
 * delimiter.
 * The prompt continuation reads the box to find where to send the body's result.
 * When a captured continuation is invoked, update the box to point to the
 * invoker's continuation, so they are composable. The resumed computation
 * delivers its result to the caller of k.apply(v) rather than the original
 * reset.
 *
 * The PromptBox is refcounted and GC-traced so it stays alive as long as
 * any captured continuation referencing it is alive.
 */

typedef struct PromptBox {
    int    refcount;
    Cont  *outer_k;   /* where the prompt currently delivers results */
} PromptBox;

static PromptBox *promptbox_new(Cont *outer_k) {
    PromptBox *pb = malloc(sizeof(PromptBox));
    pb->refcount = 1;
    pb->outer_k = cont_retain(outer_k);
    return pb;
}
static PromptBox *promptbox_retain(PromptBox *pb) {
    if (pb) pb->refcount++;
    return pb;
}
static void promptbox_release(PromptBox *pb) {
    if (pb && --pb->refcount <= 0) {
        cont_release_abandon(pb->outer_k);
        free(pb);
    }
}

/* ResetPrompt: the prompt continuation installed by reset */

typedef struct {
    Cont      base;
    PromptBox *box;
} ResetPrompt;

static void reset_prompt_trace(Cont *c) {
    ResetPrompt *rp = (ResetPrompt *)c;
    gc_trace_cont(rp->box->outer_k);
}
static void reset_prompt_cleanup(Cont *c) {
    ResetPrompt *rp = (ResetPrompt *)c;
    promptbox_release(rp->box);
}
static PendingStep *reset_prompt_apply(Cont *c, GraceObject *value) {
    ResetPrompt *rp = (ResetPrompt *)c;
    Cont *outer_k = cont_retain(rp->box->outer_k);
    cont_consumed(c);
    PendingStep *r = cont_apply(outer_k, value);
    cont_release(outer_k);
    return r;
}

/* reset(1) */

static PendingStep *prelude_reset_fn(GraceObject *self, Env *env,
                                      GraceObject **args, int nargs,
                                      Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    GraceObject *body_blk = args[0];
    /* Create prompt box and prompt continuation */
    PromptBox *box = promptbox_new(k);
    ResetPrompt *prompt = CONT_ALLOC(ResetPrompt);
    prompt->base.apply    = reset_prompt_apply;
    prompt->base.gc_trace = reset_prompt_trace;
    prompt->base.cleanup  = reset_prompt_cleanup;
    prompt->box           = promptbox_retain(box);
    /* Build env with reset_k = prompt */
    Env *reset_env = malloc(sizeof(Env));
    *reset_env = *env;
    reset_env->refcount = 1;
    cont_retain(reset_env->return_k);
    cont_retain(reset_env->except_k);
    reset_env->reset_k = cont_retain((Cont *)prompt);
    /* Evaluate body with the prompt as the continuation */
    PendingStep *r = grace_request(body_blk, reset_env, "apply(0)",
                                    NULL, 0, (Cont *)prompt);
    env_release(reset_env);
    promptbox_release(box);
    return r;
}

/* Reified continuation (the "k" passed to the shift body)  */

typedef struct {
    Cont      *captured_k;   /* continuation from shift point to prompt */
    PromptBox *box;           /* shared mutable cell for redirect on resume */
} ReifiedContData;

static void reified_cont_trace_data(void *data) {
    ReifiedContData *rd = (ReifiedContData *)data;
    gc_trace_cont(rd->captured_k);
    gc_trace_cont(rd->box->outer_k);
}
static void reified_cont_free_data(void *data) {
    ReifiedContData *rd = (ReifiedContData *)data;
    cont_release_abandon(rd->captured_k);
    promptbox_release(rd->box);
    free(rd);
}

/*
 * When k.apply(v) is called:
 *  1. Redirect the prompt box so the captured computation delivers
 *     its result to a fresh prompt wired to the caller's continuation.
 *  2. Apply the captured continuation with v.
 *
 * This makes the continuation composable: each invocation of k delivers
 * its result to the invoker, not the original reset's caller.
 */
static PendingStep *reified_cont_apply_fn(GraceObject *self, Env *env,
                                           GraceObject **args, int nargs,
                                           Cont *k, void *data) {
    (void)self; (void)env; (void)nargs;
    ReifiedContData *rd = (ReifiedContData *)data;
    GraceObject *value = args[0];
    /* Redirect the prompt box to deliver to this k */
    Cont *old_outer = rd->box->outer_k;
    rd->box->outer_k = cont_retain(k);
    cont_release(old_outer);
    /* Apply the captured continuation with the supplied value */
    return cont_apply(rd->captured_k, value);
}

/* shift(1) */

static PendingStep *prelude_shift_fn(GraceObject *self, Env *env,
                                      GraceObject **args, int nargs,
                                      Cont *k, void *data) {
    (void)self; (void)nargs; (void)data;
    GraceObject *shift_body = args[0];  /* { k -> ... } */
    /* Find the nearest reset prompt */
    if (!env->reset_k || env->reset_k == cont_done)
        grace_raise(env, "ContinuationError", "shift called outside of reset");
    ResetPrompt *prompt = (ResetPrompt *)env->reset_k;
    /* Reify the current continuation k as a Grace object with apply(1).
     * k is the continuation from the shift point to the prompt. */
    GraceObject *reified = grace_user_new(NULL);
    ReifiedContData *rd = malloc(sizeof(ReifiedContData));
    rd->captured_k = cont_retain(k);
    rd->box        = promptbox_retain(prompt->box);
    user_add_method(reified, "apply(1)", reified_cont_apply_fn, rd);
    /* Set trace_data and free_data on the method entry */
    {
        GraceUserObject *uo = (GraceUserObject *)reified;
        MethodEntry *m = uo->methods;
        while (m) {
            if (m->data == rd) {
                m->trace_data = reified_cont_trace_data;
                m->free_data  = reified_cont_free_data;
                break;
            }
            m = m->next;
        }
    }
    /* The shift body is evaluated with the prompt's outer_k as the
     * continuation, so the shift aborts to the reset boundary.
     * The result of the shift body becomes the result of the reset. */
    Cont *outer_k = cont_retain(prompt->box->outer_k);
    /* Build env without reset_k for the shift body */
    Env *shift_env = malloc(sizeof(Env));
    *shift_env = *env;
    shift_env->refcount = 1;
    cont_retain(shift_env->return_k);
    cont_retain(shift_env->except_k);
    shift_env->reset_k = NULL;  /* shift body is outside the reset */
    GraceObject *kargs[1] = { reified };
    PendingStep *r = grace_request(shift_body, shift_env, "apply(1)",
                                    kargs, 1, outer_k);
    env_release(shift_env);
    cont_release(outer_k);
    return r;
}

/* primitiveArray factory: primitiveArray.new(size) creates a mutable array */
static PendingStep *primarray_new_fn(GraceObject *self, Env *env,
                                      GraceObject **args, int nargs, Cont *k,
                                      void *data) {
    (void)self; (void)env; (void)nargs; (void)data;
    int size = (int)grace_number_val(args[0]);
    return cont_apply(k, grace_primarray_new(size));
}

GraceObject *make_prelude(void) {
    GraceObject *p = grace_user_new(NULL);

    user_add_method(p, "print(1)",       prelude_print_fn,           NULL);
    user_add_method(p, "true(0)",        true_fn,                    NULL);
    user_add_method(p, "false(0)",       false_fn,                   NULL);
    user_add_method(p, "Exception(0)",   prelude_exception_fn,       NULL);
    user_add_method(p, "Number(1)",      prelude_number_fn,          NULL);
    user_add_method(p, "String(1)",      prelude_string_fn,          NULL);
    user_add_method(p, "while(1)do(1)",  prelude_while_do_fn,        NULL);
    user_add_method(p, "for(1)do(1)",    prelude_for_do_fn,          NULL);
    user_add_method(p, "try(1)catch(1)", prelude_try_catch_fn,       NULL);
    user_add_method(p, "getFileContents(1)", prelude_getFileContents_fn, NULL);

    /* if/then variants */
    user_add_method(p, "if(1)then(1)",          prelude_if_then_fn,      NULL);
    user_add_method(p, "if(1)then(1)else(1)",   prelude_if_then_else_fn, NULL);
    /* elseif variants with final else */
    user_add_method(p, "if(1)then(1)elseif(1)then(1)else(1)",                             prelude_if_elseif_else_fn, NULL);
    user_add_method(p, "if(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)",             prelude_if_elseif_else_fn, NULL);
    user_add_method(p, "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)", prelude_if_elseif_else_fn, NULL);
    /* elseif variants without final else */
    user_add_method(p, "if(1)then(1)elseif(1)then(1)",                    prelude_if_elseif_fn, NULL);
    user_add_method(p, "if(1)then(1)elseif(1)then(1)elseif(1)then(1)",    prelude_if_elseif_fn, NULL);

    /* match/case variants: match(1)case(N) */
    user_add_method(p, "match(1)case(1)",       build_match_fn, NULL);
    user_add_method(p, "match(1)case(1)case(1)", build_match_fn, NULL);
    user_add_method(p, "match(1)case(1)case(1)case(1)", build_match_fn, NULL);
    user_add_method(p, "match(1)case(1)case(1)case(1)case(1)", build_match_fn, NULL);

    /* Bind true/false/done as direct values too */
    user_bind_def(p, "true",  grace_true);
    user_bind_def(p, "false", grace_false);
    user_bind_def(p, "done",  grace_done);

    /* delimited continuations */
    user_add_method(p, "reset(1)",  prelude_reset_fn,  NULL);
    user_add_method(p, "shift(1)",  prelude_shift_fn,  NULL);

    /* primitiveArray factory */
    GraceObject *pa_factory = grace_user_new(NULL);
    user_add_method(pa_factory, "new(1)", primarray_new_fn, NULL);
    user_bind_def(p, "primitiveArray", pa_factory);

    return p;
}
