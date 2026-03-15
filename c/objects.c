/* objects.c - GraceObject type implementations */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "ast.h"
#include "grace.h"
#include "gc.h"

extern PendingStep *eval_stmts(ASTNode *stmts, Env *env, Cont *k);
extern PendingStep *eval_node(ASTNode *node, Env *env, Cont *k);

static GraceObject *grace_patternor_new(GraceObject *left, GraceObject *right);

/* VTable forward declarations */
const GraceVTable grace_number_vtable;
const GraceVTable grace_string_vtable;
const GraceVTable grace_bool_vtable;
const GraceVTable grace_block_vtable;
const GraceVTable grace_user_vtable;
const GraceVTable grace_exception_vtable;

/* match-result helper */
static GraceObject *make_match_result(int succeeded, GraceObject *result) {
    GraceObject *obj = grace_user_new(NULL);
    user_bind_def(obj, "succeeded", grace_bool(succeeded));
    user_bind_def(obj, "result",    result);
    return obj;
}

/*  Sentinel singletons  */
static PendingStep *sentinel_request(GraceObject *self, Env *env,
                                      const char *name,
                                      GraceObject **args, int nargs, Cont *k) {
    (void)args;(void)nargs;(void)env;
    if(strcmp(name,"asString(0)")==0)
        return cont_apply(k,grace_string_new((self==grace_done)?"done":"uninitialised"));
    if(strcmp(name,"==(1)")==0) return cont_apply(k, grace_bool(self == args[0]));
    if(strcmp(name,"!=(1)")==0) return cont_apply(k, grace_bool(self != args[0]));
    grace_fatal("Cannot call '%s' on sentinel", name);
}
static const char *sentinel_describe(GraceObject *self) {
    return (self==grace_done)?"done":"uninitialised";
}
static const GraceVTable sentinel_vtable = { sentinel_request, sentinel_describe, NULL, NULL };
static GraceObject _grace_done_obj   = { &sentinel_vtable, NULL, NULL, GC_STATIC, 0 };
static GraceObject _grace_uninit_obj = { &sentinel_vtable, NULL, NULL, GC_STATIC, 0 };
GraceObject *grace_done   = &_grace_done_obj;
GraceObject *grace_uninit = &_grace_uninit_obj;

/*  Range object  */
typedef struct { double start, end, step; } RangeData;
typedef struct { double current, end_val, step; GraceObject *block; Env *env; Cont *k; } RangeIterState;
typedef struct { Cont base; RangeIterState *st; } RangeIterCont;
GraceObject *grace_range_new(double start, double end, double step);

static void range_iter_trace(Cont *c) {
    RangeIterCont *rc = (RangeIterCont *)c;
    RangeIterState *st = rc->st;
    if (!st) return;
    gc_mark_grey(st->block);
    gc_trace_env(st->env);
    gc_trace_cont(st->k);
}
static void range_iter_cleanup(Cont *c) {
    RangeIterCont *rc = (RangeIterCont *)c;
    RangeIterState *st = rc->st;
    if (!st) return;
    env_release(st->env);
    cont_release_abandon(st->k);
    free(st);
}
static PendingStep *range_iter_next(Cont *c, GraceObject *v) {
    (void)v;
    RangeIterCont *rc = (RangeIterCont *)c;
    RangeIterState *st = rc->st;
    st->current += st->step;
    if ((st->step > 0 && st->current > st->end_val) || (st->step < 0 && st->current < st->end_val)) {
        Cont *k = cont_retain(st->k);
        rc->st = NULL;
        cont_consumed(c);
        env_release(st->env);
        cont_release(st->k);
        free(st);
        PendingStep *r = cont_apply(k, grace_done);
        cont_release(k);
        return r;
    }
    GraceObject *num = grace_number_new(st->current);
    GraceObject *a[1] = {num};
    RangeIterCont *nc = CONT_ALLOC(RangeIterCont);
    nc->base.apply = range_iter_next; nc->base.gc_trace = range_iter_trace;
    nc->base.cleanup = range_iter_cleanup; nc->st = st;
    rc->st = NULL;
    cont_consumed(c);
    return grace_request(st->block, st->env, "apply(1)", a, 1, (Cont *)nc);
}

static PendingStep *range_do_fn(GraceObject *self, Env *env, GraceObject **args,
                                 int nargs, Cont *k, void *data) {
    (void)self;(void)nargs;
    RangeData *d = (RangeData *)data;
    if (d->step > 0 && d->start > d->end) return cont_apply(k, grace_done);
    if (d->step < 0 && d->start < d->end) return cont_apply(k, grace_done);
    RangeIterState *st = malloc(sizeof(RangeIterState));
    st->current = d->start; st->end_val = d->end; st->step = d->step;
    st->block = args[0]; st->env = env_retain(env); st->k = cont_retain(k);
    GraceObject *num = grace_number_new(st->current);
    GraceObject *a[1] = {num};
    RangeIterCont *rc = CONT_ALLOC(RangeIterCont);
    rc->base.apply = range_iter_next; rc->base.gc_trace = range_iter_trace;
    rc->base.cleanup = range_iter_cleanup; rc->st = st;
    return grace_request(args[0], env, "apply(1)", a, 1, (Cont *)rc);
}

typedef struct { double current, start_val, step; GraceObject *block; Env *env; Cont *k; } RevIterState;
typedef struct { Cont base; RevIterState *st; } RevIterCont;

static void rev_iter_trace(Cont *c) {
    RevIterCont *rc = (RevIterCont *)c;
    RevIterState *st = rc->st;
    if (!st) return;
    gc_mark_grey(st->block);
    gc_trace_env(st->env);
    gc_trace_cont(st->k);
}
static void rev_iter_cleanup(Cont *c) {
    RevIterCont *rc = (RevIterCont *)c;
    RevIterState *st = rc->st;
    if (!st) return;
    env_release(st->env);
    cont_release_abandon(st->k);
    free(st);
}
static PendingStep *rev_iter_next(Cont *c, GraceObject *v) {
    (void)v;
    RevIterCont *rc = (RevIterCont *)c;
    RevIterState *st = rc->st;
    st->current -= st->step;
    if ((st->step > 0 && st->current < st->start_val) || (st->step < 0 && st->current > st->start_val)) {
        Cont *k = cont_retain(st->k);
        rc->st = NULL;
        cont_consumed(c);
        env_release(st->env);
        cont_release(st->k);
        free(st);
        PendingStep *r = cont_apply(k, grace_done);
        cont_release(k);
        return r;
    }
    GraceObject *num = grace_number_new(st->current);
    GraceObject *a[1] = {num};
    RevIterCont *nc = CONT_ALLOC(RevIterCont);
    nc->base.apply = rev_iter_next; nc->base.gc_trace = rev_iter_trace;
    nc->base.cleanup = rev_iter_cleanup; nc->st = st;
    rc->st = NULL;
    cont_consumed(c);
    return grace_request(st->block, st->env, "apply(1)", a, 1, (Cont *)nc);
}

static PendingStep *range_revdo_fn(GraceObject *self, Env *env, GraceObject **args,
                                    int nargs, Cont *k, void *data) {
    (void)self;(void)nargs;
    RangeData *d = (RangeData *)data;
    if (d->step > 0 && d->end < d->start) return cont_apply(k, grace_done);
    if (d->step < 0 && d->end > d->start) return cont_apply(k, grace_done);
    RevIterState *st = malloc(sizeof(RevIterState));
    st->current = d->end; st->start_val = d->start; st->step = d->step;
    st->block = args[0]; st->env = env_retain(env); st->k = cont_retain(k);
    GraceObject *num = grace_number_new(st->current);
    GraceObject *a[1] = {num};
    RevIterCont *rc = CONT_ALLOC(RevIterCont);
    rc->base.apply = rev_iter_next; rc->base.gc_trace = rev_iter_trace;
    rc->base.cleanup = rev_iter_cleanup; rc->st = st;
    return grace_request(args[0], env, "apply(1)", a, 1, (Cont *)rc);
}

static PendingStep *range_asstr_fn(GraceObject *self, Env *env, GraceObject **args,
                                    int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)args;(void)nargs;
    RangeData *d = (RangeData *)data;
    return cont_apply(k, grace_string_take(str_fmt("%.0f..%.0f", d->start, d->end)));
}

static PendingStep *range_range_fn(GraceObject *self, Env *env, GraceObject **args,
                                    int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)nargs;
    RangeData *d = (RangeData *)data;
    double step = grace_number_val(args[0]);
    return cont_apply(k, grace_range_new(d->start, d->end, step));
}

GraceObject *grace_range_new(double start, double end, double step) {
    GraceObject *range = grace_user_new(NULL);
    user_bind_def(range, "start", grace_number_new(start));
    user_bind_def(range, "end",   grace_number_new(end));
    user_bind_def(range, "step",  grace_number_new(step));
    RangeData *rd = malloc(sizeof(RangeData));
    rd->start = start; rd->end = end; rd->step = step;
    user_add_method(range, "do(1)",        range_do_fn,    rd);
    user_add_method(range, "reverseDo(1)", range_revdo_fn, rd);
    user_add_method(range, "asString(0)",  range_asstr_fn, rd);
    user_add_method(range, "..(1)", range_range_fn, rd);
    /* Set free_data on the range method entries (RangeData is shared,
     * only the first one frees it) */
    GraceUserObject *uo = (GraceUserObject *)range;
    for (MethodEntry *m = uo->methods; m; m = m->next) {
        if (m->data == rd && m->fn == range_do_fn) {
            m->free_data = free;  /* only the first one frees RangeData */
        }
    }
    return range;
}

/*  GraceNumber  */
static PendingStep *number_request(GraceObject *self, Env *env,
                                    const char *name,
                                    GraceObject **args, int nargs, Cont *k) {
    (void)nargs;
    double v = ((GraceNumber *)self)->value;
#define N1 grace_number_val(args[0])
    if (strcmp(name,"+(1)")==0)   return cont_apply(k, grace_number_new(v + N1));
    if (strcmp(name,"-(1)")==0)   return cont_apply(k, grace_number_new(v - N1));
    if (strcmp(name,"*(1)")==0)   return cont_apply(k, grace_number_new(v * N1));
    if (strcmp(name,"/(1)")==0) {
        double d = N1;
        if (d == 0.0) grace_raise(env, "ZeroDivision", "Division by zero");
        return cont_apply(k, grace_number_new(v / d));
    }
    if (strcmp(name,"%(1)")==0)    return cont_apply(k, grace_number_new(fmod(v, N1)));
    if (strcmp(name,"prefix-(0)")==0) return cont_apply(k, grace_number_new(-v));
    if (strcmp(name,">(1)")==0)    return cont_apply(k, grace_bool(v >  N1));
    if (strcmp(name,"<(1)")==0)    return cont_apply(k, grace_bool(v <  N1));
    if (strcmp(name,">=(1)")==0)   return cont_apply(k, grace_bool(v >= N1));
    if (strcmp(name,"<=(1)")==0)   return cont_apply(k, grace_bool(v <= N1));
    if (strcmp(name,"==(1)")==0)   return cont_apply(k, grace_bool(v == N1));
    if (strcmp(name,"!=(1)")==0)   return cont_apply(k, grace_bool(v != N1));
#undef N1
    if (strcmp(name,"truncated(0)")==0) return cont_apply(k, grace_number_new(trunc(v)));
    if (strcmp(name,"rounded(0)")==0)   return cont_apply(k, grace_number_new(round(v)));
    if (strcmp(name,"floor(0)")==0)     return cont_apply(k, grace_number_new(floor(v)));
    if (strcmp(name,"ceiling(0)")==0)   return cont_apply(k, grace_number_new(ceil(v)));
    if (strcmp(name,"abs(0)")==0)       return cont_apply(k, grace_number_new(fabs(v)));
    if (strcmp(name,"sqrt(0)")==0)      return cont_apply(k, grace_number_new(sqrt(v)));
    if (strcmp(name,"asInteger(0)")==0) return cont_apply(k, grace_number_new(trunc(v)));
    if (strcmp(name,"..(1)")==0)        return cont_apply(k, grace_range_new(v, grace_number_val(args[0]), 1.0));
    if (strcmp(name,"asString(0)")==0 || strcmp(name,"asDebugString(0)")==0) {
        char buf[64];
        if (v == trunc(v) && fabs(v) < 1e15)
            snprintf(buf, sizeof(buf), "%.0f", v);
        else
            snprintf(buf, sizeof(buf), "%g", v);
        return cont_apply(k, grace_string_new(buf));
    }
    if (strcmp(name,"hash(0)")==0) {
        union { double d; uint64_t u; } bits;
        bits.d = v;
        uint32_t hash = (uint32_t)(bits.u ^ (bits.u >> 32));
        return cont_apply(k, grace_number_new((double)hash));
    }
    if (strcmp(name,"match(1)")==0) {
        if (args[0]->vt == &grace_number_vtable)
            return cont_apply(k, make_match_result(((GraceNumber*)args[0])->value == v, args[0]));
        return cont_apply(k, make_match_result(0, grace_done));
    }
    if (strcmp(name,"|(1)")==0) return cont_apply(k, grace_patternor_new(self, args[0]));
    grace_fatal("No method '%s' on Number(%g)", name, v);
}
static const char *number_describe(GraceObject *self) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "Number(%g)", ((GraceNumber*)self)->value);
    return buf;
}
const GraceVTable grace_number_vtable = { number_request, number_describe, NULL, NULL };
GraceObject *grace_number_new(double v) {
    GraceNumber *n = (GraceNumber *)gc_alloc(sizeof(GraceNumber));
    n->base.vt = &grace_number_vtable; n->value = v;
    return (GraceObject *)n;
}
double grace_number_val(GraceObject *o) {
    if (o->vt != &grace_number_vtable)
        grace_fatal("Expected Number, got %s", o->vt->describe(o));
    return ((GraceNumber *)o)->value;
}

/*  GraceString  */
/* Continuation for string ++ <arbitrary object>: receives asString result */
typedef struct {
    Cont        base;
    GraceObject *left_str;  /* left-hand GraceString, kept alive for GC */
    const char  *prefix;    /* points into left_str->value */
    Cont        *k;
} ConcatAsStrCont;
static void concat_asstr_trace(Cont *c) {
    ConcatAsStrCont *cc = (ConcatAsStrCont *)c;
    gc_mark_grey(cc->left_str);
    gc_trace_cont(cc->k);
}
static void concat_asstr_cleanup(Cont *c) {
    ConcatAsStrCont *cc = (ConcatAsStrCont *)c;
    cont_release_abandon(cc->k);
}
static PendingStep *concat_asstr_apply(Cont *c, GraceObject *v) {
    ConcatAsStrCont *cc = (ConcatAsStrCont *)c;
    const char *rhs = (v && v->vt == &grace_string_vtable)
                    ? grace_string_val(v) : "";
    Cont *k = cont_retain(cc->k);
    cont_consumed(c);
    PendingStep *r = cont_apply(k, grace_string_take(str_cat(cc->prefix, rhs)));
    cont_release(k);
    return r;
}

static PendingStep *string_request(GraceObject *self, Env *env,
                                    const char *name,
                                    GraceObject **args, int nargs, Cont *k) {
    (void)nargs;
    const char *v = ((GraceString *)self)->value;
    size_t len = strlen(v);
    if (strcmp(name,"++(1)")==0) {
        const char *rhs;
        char nbuf[64];
        if (args[0]->vt == &grace_string_vtable) {
            rhs = grace_string_val(args[0]);
        } else if (args[0]->vt == &grace_number_vtable) {
            double n = ((GraceNumber*)args[0])->value;
            long long ni = (long long)n;
            if ((double)ni == n) snprintf(nbuf, sizeof(nbuf), "%lld", ni);
            else snprintf(nbuf, sizeof(nbuf), "%g", n);
            rhs = nbuf;
        } else if (args[0]->vt == &grace_bool_vtable) {
            rhs = ((GraceBool*)args[0])->value ? "true" : "false";
        } else {
            /* Arbitrary object: request asString(0) then concatenate */
            ConcatAsStrCont *cc = CONT_ALLOC(ConcatAsStrCont);
            cc->base.apply = concat_asstr_apply;
            cc->base.gc_trace = concat_asstr_trace;
            cc->base.cleanup = concat_asstr_cleanup;
            cc->left_str   = self;
            cc->prefix     = v;
            cc->k          = cont_retain(k);
            return grace_request(args[0], env, "asString(0)", NULL, 0, (Cont *)cc);
        }
        return cont_apply(k, grace_string_take(str_cat(v, rhs)));
    }
    if (strcmp(name,"size(0)")==0)
        return cont_apply(k, grace_number_new((double)len));
    if (strcmp(name,"at(1)")==0) {
        int idx = (int)grace_number_val(args[0]) - 1;
        if (idx < 0 || idx >= (int)len)
            grace_raise(env, "BoundsError", "String index %d out of bounds", idx+1);
        char ch[2] = { v[idx], 0 };
        return cont_apply(k, grace_string_new(ch));
    }
    if (strcmp(name,"firstCodepoint(0)")==0) {
        if (len == 0) grace_raise(env, "BoundsError", "Empty string");
        return cont_apply(k, grace_number_new((double)(unsigned char)v[0]));
    }
    if (strcmp(name,"substringFrom(1)to(1)")==0) {
        int from = (int)grace_number_val(args[0]) - 1;
        int to   = (int)grace_number_val(args[1]);
        if (from < 0) from = 0;
        if (to > (int)len) to = (int)len;
        if (from >= to) return cont_apply(k, grace_string_new(""));
        char *sub = malloc(to - from + 1);
        memcpy(sub, v + from, to - from); sub[to - from] = 0;
        return cont_apply(k, grace_string_take(sub));
    }
    if (strcmp(name,"replace(1)with(1)")==0) {
        const char *pat = grace_string_val(args[0]);
        const char *rep = grace_string_val(args[1]);
        size_t plen = strlen(pat), rlen = strlen(rep);
        if (plen == 0) return cont_apply(k, grace_string_new(v));
        size_t count = 0; const char *p = v;
        while ((p = strstr(p, pat)) != NULL) { count++; p += plen; }
        size_t newlen = len + count * rlen - count * plen;
        char *res = malloc(newlen + 1), *out = res;
        p = v; const char *found;
        while ((found = strstr(p, pat)) != NULL) {
            size_t pre = found - p; memcpy(out, p, pre); out += pre;
            memcpy(out, rep, rlen); out += rlen; p = found + plen;
        }
        strcpy(out, p);
        return cont_apply(k, grace_string_take(res));
    }
    if (strcmp(name,"==(1)")==0) return cont_apply(k, grace_bool(strcmp(v, grace_string_val(args[0])) == 0));
    if (strcmp(name,"!=(1)")==0) return cont_apply(k, grace_bool(strcmp(v, grace_string_val(args[0])) != 0));
    if (strcmp(name,"<(1)")==0)  return cont_apply(k, grace_bool(strcmp(v, grace_string_val(args[0])) <  0));
    if (strcmp(name,">(1)")==0)  return cont_apply(k, grace_bool(strcmp(v, grace_string_val(args[0])) >  0));
    if (strcmp(name,"<=(1)")==0) return cont_apply(k, grace_bool(strcmp(v, grace_string_val(args[0])) <= 0));
    if (strcmp(name,">=(1)")==0) return cont_apply(k, grace_bool(strcmp(v, grace_string_val(args[0])) >= 0));
    if (strcmp(name,"asString(0)")==0) return cont_apply(k, self);
    if (strcmp(name,"asDebugString(0)")==0) {
        /* Escape special characters and wrap in double quotes */
        size_t esc_len = 2; /* opening and closing quote */
        for (size_t i = 0; i < len; i++) {
            switch (v[i]) {
                case '\\': case '\n': case '\t': case '\r': case '"': esc_len += 2; break;
                default: esc_len += 1; break;
            }
        }
        char *esc = malloc(esc_len + 1);
        char *p = esc;
        *p++ = '"';
        for (size_t i = 0; i < len; i++) {
            switch (v[i]) {
                case '\\': *p++ = '\\'; *p++ = '\\'; break;
                case '\n':  *p++ = '\\'; *p++ = 'n'; break;
                case '\t':  *p++ = '\\'; *p++ = 't'; break;
                case '\r':  *p++ = '\\'; *p++ = 'r'; break;
                case '"':  *p++ = '\\'; *p++ = '"'; break;
                default:    *p++ = v[i]; break;
            }
        }
        *p++ = '"';
        *p = '\0';
        return cont_apply(k, grace_string_take(esc));
    }
    if (strcmp(name,"hash(0)")==0) {
        unsigned long hash = 5381;
        for (const char *p = v; *p; p++)
            hash = ((hash << 5) + hash) + (unsigned char)*p;
        return cont_apply(k, grace_number_new((double)(uint32_t)hash));
    }
    if (strcmp(name,"asNumber(0)")==0) {
        char *end; double num = strtod(v, &end);
        if (end == v) grace_raise(env, "TypeError", "Not a number: '%s'", v);
        return cont_apply(k, grace_number_new(num));
    }
    if (strcmp(name,"startsWith(1)")==0) {
        const char *pre = grace_string_val(args[0]);
        return cont_apply(k, grace_bool(strncmp(v, pre, strlen(pre)) == 0));
    }
    if (strcmp(name,"endsWith(1)")==0) {
        const char *suf = grace_string_val(args[0]);
        size_t sl = strlen(suf);
        return cont_apply(k, grace_bool(len >= sl && strcmp(v + len - sl, suf) == 0));
    }
    if (strcmp(name,"contains(1)")==0)
        return cont_apply(k, grace_bool(strstr(v, grace_string_val(args[0])) != NULL));
    if (strcmp(name,"trimmed(0)")==0) {
        const char *s = v;
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        const char *e = v + len;
        while (e > s && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\n' || e[-1] == '\r')) e--;
        size_t tl = e - s; char *t = malloc(tl + 1);
        memcpy(t, s, tl); t[tl] = 0;
        return cont_apply(k, grace_string_take(t));
    }
    if (strcmp(name,"reversed(0)")==0) {
        char *r = malloc(len + 1);
        for (size_t i = 0; i < len; i++) r[i] = v[len-1-i];
        r[len] = 0;
        return cont_apply(k, grace_string_take(r));
    }
    if (strcmp(name,"match(1)")==0) {
        if (args[0]->vt == &grace_string_vtable)
            return cont_apply(k, make_match_result(
                strcmp(((GraceString*)args[0])->value, v) == 0, args[0]));
        return cont_apply(k, make_match_result(0, grace_done));
    }
    if (strcmp(name,"|(1)")==0) return cont_apply(k, grace_patternor_new(self, args[0]));
    grace_fatal("No method '%s' on String(\"%s\")", name, v);
}
static const char *string_describe(GraceObject *self) {
    static char buf[64];
    snprintf(buf, sizeof(buf), "String(\"%s\")", ((GraceString*)self)->value);
    return buf;
}
static void string_sweep_free(GraceObject *self) {
    GraceString *gs = (GraceString *)self;
    if (gs->value) free((void *)gs->value);
}
const GraceVTable grace_string_vtable = { string_request, string_describe, NULL, string_sweep_free };
GraceObject *grace_string_new(const char *s) {
    GraceString *gs = (GraceString *)gc_alloc(sizeof(GraceString));
    gs->base.vt = &grace_string_vtable;
    gs->value = str_dup(s ? s : "");
    return (GraceObject *)gs;
}
/* Like grace_string_new, but takes ownership of the malloc'd string `s`
 * instead of duplicating it.  Caller must not free `s` afterwards. */
GraceObject *grace_string_take(char *s) {
    GraceString *gs = (GraceString *)gc_alloc(sizeof(GraceString));
    gs->base.vt = &grace_string_vtable;
    gs->value = s ? s : str_dup("");
    return (GraceObject *)gs;
}
const char *grace_string_val(GraceObject *o) {
    if (o->vt != &grace_string_vtable)
        grace_fatal("Expected String, got %s", o->vt->describe(o));
    return ((GraceString *)o)->value;
}

/*  GraceBool  */
static PendingStep *apply_lazy(GraceObject *arg, Env *env, Cont *k) {
    if (arg->vt == &grace_block_vtable)
        return grace_request(arg, env, "apply(0)", NULL, 0, k);
    return cont_apply(k, arg);
}

static PendingStep *bool_request(GraceObject *self, Env *env, const char *name,
                                  GraceObject **args, int nargs, Cont *k) {
    (void)nargs;
    int v = ((GraceBool *)self)->value;
    if (strcmp(name,"prefix!(0)")==0 || strcmp(name,"prefix!")==0 || strcmp(name,"not(0)")==0)
        return cont_apply(k, grace_bool(!v));
    if (strcmp(name,"&&(1)")==0 || strcmp(name,"and(1)")==0) {
        if (!v) return cont_apply(k, grace_false);
        return apply_lazy(args[0], env, k);
    }
    if (strcmp(name,"||(1)")==0 || strcmp(name,"or(1)")==0) {
        if (v) return cont_apply(k, grace_true);
        return apply_lazy(args[0], env, k);
    }
    if (strcmp(name,"&(1)")==0) {
        if (!v) return cont_apply(k, grace_false);
        if (args[0]->vt == &grace_bool_vtable) return cont_apply(k, grace_bool(grace_bool_val(args[0])));
        return apply_lazy(args[0], env, k);
    }
    if (strcmp(name,"|(1)")==0) {
        if (v) return cont_apply(k, grace_true);
        if (args[0]->vt == &grace_bool_vtable) return cont_apply(k, grace_bool(grace_bool_val(args[0])));
        return apply_lazy(args[0], env, k);
    }
    if (strcmp(name,"==(1)")==0)
        return cont_apply(k, grace_bool(args[0]->vt == &grace_bool_vtable && grace_bool_val(args[0]) == v));
    if (strcmp(name,"!=(1)")==0)
        return cont_apply(k, grace_bool(!(args[0]->vt == &grace_bool_vtable && grace_bool_val(args[0]) == v)));
    if (strcmp(name,"asString(0)")==0 || strcmp(name,"asDebugString(0)")==0)
        return cont_apply(k, grace_string_new(v ? "true" : "false"));
    if (strcmp(name,"hash(0)")==0)
        return cont_apply(k, grace_number_new(v ? 3.0 : 7.0));
    if (strcmp(name,"ifTrue(1)")==0)           { if (v)  return apply_lazy(args[0], env, k); return cont_apply(k, grace_done); }
    if (strcmp(name,"ifFalse(1)")==0)          { if (!v) return apply_lazy(args[0], env, k); return cont_apply(k, grace_done); }
    if (strcmp(name,"ifTrue(1)ifFalse(1)")==0) return apply_lazy(v ? args[0] : args[1], env, k);
    if (strcmp(name,"ifFalse(1)ifTrue(1)")==0) return apply_lazy(v ? args[1] : args[0], env, k);
    if (strcmp(name,"match(1)")==0) {
        if (args[0]->vt == &grace_bool_vtable)
            return cont_apply(k, make_match_result(grace_bool_val(args[0]) == v, args[0]));
        return cont_apply(k, make_match_result(0, grace_done));
    }
    grace_fatal("No method '%s' on Boolean(%s)", name, v ? "true" : "false");
}
static const char *bool_describe(GraceObject *self) {
    return ((GraceBool *)self)->value ? "true" : "false";
}
const GraceVTable grace_bool_vtable = { bool_request, bool_describe, NULL, NULL };
static GraceBool _grace_true_obj  = { { &grace_bool_vtable, NULL, NULL, GC_STATIC, 0 }, 1 };
static GraceBool _grace_false_obj = { { &grace_bool_vtable, NULL, NULL, GC_STATIC, 0 }, 0 };
GraceObject *grace_true  = (GraceObject *)&_grace_true_obj;
GraceObject *grace_false = (GraceObject *)&_grace_false_obj;
int grace_bool_val(GraceObject *o) {
    if (o->vt != &grace_bool_vtable)
        grace_fatal("Expected Bool, got %s", o->vt->describe(o));
    return ((GraceBool *)o)->value;
}

/*  GracePatternOr  */
typedef struct {
    GraceObject  base;
    GraceObject *left;
    GraceObject *right;
} GracePatternOr;

typedef struct {
    Cont         base;
    GraceObject *right;
    GraceObject *target;
    Env         *env;
    Cont        *k;
} PatternOrLeftCont;

static void por_left_trace(Cont *c) {
    PatternOrLeftCont *pc = (PatternOrLeftCont *)c;
    gc_mark_grey(pc->right);
    gc_mark_grey(pc->target);
    gc_trace_env(pc->env);
    gc_trace_cont(pc->k);
}
static void por_left_cleanup(Cont *c) {
    PatternOrLeftCont *pc = (PatternOrLeftCont *)c;
    env_release(pc->env);
    cont_release_abandon(pc->k);
}
static PendingStep *por_left_apply(Cont *c, GraceObject *result) {
    PatternOrLeftCont *pc = (PatternOrLeftCont *)c;
    GraceObject *right = pc->right;
    GraceObject *target = pc->target;
    Env *env = pc->env;
    Cont *k = pc->k;
    pc->env = NULL; pc->k = NULL;
    cont_consumed(c);
    gc_push_cont_root(&k);
    GraceObject *succ = grace_request_sync(result, env, "succeeded(0)", NULL, 0);
    gc_pop_cont_root();
    if (succ->vt == &grace_bool_vtable && grace_bool_val(succ)) {
        PendingStep *r = cont_apply(k, result);
        env_release(env); cont_release(k);
        return r;
    }
    GraceObject *a[1] = { target };
    PendingStep *r = grace_request(right, env, "match(1)", a, 1, k);
    env_release(env); cont_release(k);
    return r;
}

static PendingStep *patternor_request(GraceObject *self, Env *env, const char *name,
                                       GraceObject **args, int nargs, Cont *k) {
    GracePatternOr *po = (GracePatternOr *)self;
    (void)nargs;
    if (strcmp(name, "match(1)") == 0) {
        PatternOrLeftCont *pc = CONT_ALLOC(PatternOrLeftCont);
        pc->base.apply = por_left_apply;
        pc->base.gc_trace = por_left_trace;
        pc->base.cleanup = por_left_cleanup;
        pc->right = po->right;
        pc->target = args[0];
        pc->env = env_retain(env);
        pc->k = cont_retain(k);
        GraceObject *a[1] = { args[0] };
        return grace_request(po->left, env, "match(1)", a, 1, (Cont *)pc);
    }
    if (strcmp(name, "|(1)") == 0)
        return cont_apply(k, grace_patternor_new(self, args[0]));
    grace_fatal("No method '%s' on PatternOr", name);
}
static const char *patternor_describe(GraceObject *self) { (void)self; return "PatternOr"; }
static void patternor_trace(GraceObject *self) {
    GracePatternOr *po = (GracePatternOr *)self;
    gc_mark_grey(po->left);
    gc_mark_grey(po->right);
}
static const GraceVTable grace_patternor_vtable = {
    patternor_request, patternor_describe, patternor_trace, NULL
};
static GraceObject *grace_patternor_new(GraceObject *left, GraceObject *right) {
    GracePatternOr *po = (GracePatternOr *)gc_alloc(sizeof(GracePatternOr));
    po->base.vt = &grace_patternor_vtable;
    po->left = left;
    po->right = right;
    return (GraceObject *)po;
}

/*  GraceBlock  */
typedef struct {
    PendingStep base;
    ASTNode    *body;
    Env        *inner_env;
    Cont       *k;
} BlockBodyStep;

static void block_body_step_trace(PendingStep *self) {
    BlockBodyStep *s = (BlockBodyStep *)self;
    gc_trace_env(s->inner_env);
    gc_trace_cont(s->k);
}
static PendingStep *block_body_go(PendingStep *self) {
    BlockBodyStep *s = (BlockBodyStep *)self;
    ASTNode *body = s->body;
    Env *inner = s->inner_env;
    Cont *k = s->k;
    free(self);
    PendingStep *r = eval_stmts(body, inner, k);
    cont_release(k);
    env_release(inner);
    return r;
}

/* Block match continuations */

static PendingStep *block_request(GraceObject *self, Env *env, const char *name,
                                   GraceObject **args, int nargs, Cont *k);

/* Wrap body result in MatchResult(true, result) */
typedef struct { Cont base; Cont *k; } BlockMatchWrapCont;
static void bmw_trace(Cont *c)   { gc_trace_cont(((BlockMatchWrapCont *)c)->k); }
static void bmw_cleanup(Cont *c) { cont_release_abandon(((BlockMatchWrapCont *)c)->k); }
static PendingStep *bmw_apply(Cont *c, GraceObject *body_result) {
    BlockMatchWrapCont *bc = (BlockMatchWrapCont *)c;
    Cont *k = bc->k; bc->k = NULL;
    cont_consumed(c);
    PendingStep *r = cont_apply(k, make_match_result(1, body_result));
    cont_release(k);
    return r;
}

/* After pattern.match(target) returns, check succeeded; if yes apply body */
typedef struct {
    Cont         base;
    GraceObject *blk;
    GraceObject *target;
    Env         *env;
    Cont        *k;
} BlockMatchCheckCont;
static void bmc_trace(Cont *c) {
    BlockMatchCheckCont *bc = (BlockMatchCheckCont *)c;
    gc_mark_grey(bc->blk);
    gc_mark_grey(bc->target);
    gc_trace_env(bc->env);
    gc_trace_cont(bc->k);
}
static void bmc_cleanup(Cont *c) {
    BlockMatchCheckCont *bc = (BlockMatchCheckCont *)c;
    env_release(bc->env);
    cont_release_abandon(bc->k);
}
static PendingStep *bmc_apply(Cont *c, GraceObject *match_result) {
    BlockMatchCheckCont *bc = (BlockMatchCheckCont *)c;
    GraceObject *blk = bc->blk;
    GraceObject *target = bc->target;
    Env *env = bc->env;
    Cont *k = bc->k;
    bc->env = NULL; bc->k = NULL;
    cont_consumed(c);
    gc_push_cont_root(&k);
    GraceObject *succ = grace_request_sync(match_result, env, "succeeded(0)", NULL, 0);
    gc_pop_cont_root();
    if (succ->vt == &grace_bool_vtable && grace_bool_val(succ)) {
        BlockMatchWrapCont *wc = CONT_ALLOC(BlockMatchWrapCont);
        wc->base.apply = bmw_apply;
        wc->base.gc_trace = bmw_trace;
        wc->base.cleanup = bmw_cleanup;
        wc->k = k; /* transfer ownership */
        GraceObject *a[1] = { target };
        PendingStep *r = block_request(blk, env, "apply(1)", a, 1, (Cont *)wc);
        env_release(env);
        return r;
    }
    PendingStep *r = cont_apply(k, match_result);
    env_release(env); cont_release(k);
    return r;
}

/* After pattern expression is evaluated, call pattern.match(target) */
typedef struct {
    Cont         base;
    GraceObject *blk;
    GraceObject *target;
    Env         *env;
    Cont        *k;
} BlockMatchPatternCont;
static void bmp_trace(Cont *c) {
    BlockMatchPatternCont *bc = (BlockMatchPatternCont *)c;
    gc_mark_grey(bc->blk);
    gc_mark_grey(bc->target);
    gc_trace_env(bc->env);
    gc_trace_cont(bc->k);
}
static void bmp_cleanup(Cont *c) {
    BlockMatchPatternCont *bc = (BlockMatchPatternCont *)c;
    env_release(bc->env);
    cont_release_abandon(bc->k);
}
static PendingStep *bmp_apply(Cont *c, GraceObject *pattern) {
    BlockMatchPatternCont *bc = (BlockMatchPatternCont *)c;
    GraceObject *blk = bc->blk;
    GraceObject *target = bc->target;
    Env *env = bc->env;
    Cont *k = bc->k;
    bc->env = NULL; bc->k = NULL;
    cont_consumed(c);
    BlockMatchCheckCont *chk = CONT_ALLOC(BlockMatchCheckCont);
    chk->base.apply = bmc_apply;
    chk->base.gc_trace = bmc_trace;
    chk->base.cleanup = bmc_cleanup;
    chk->blk = blk;
    chk->target = target;
    chk->env = env;  /* transfer ownership */
    chk->k = k;      /* transfer ownership */
    GraceObject *a[1] = { target };
    return grace_request(pattern, env, "match(1)", a, 1, (Cont *)chk);
}

static PendingStep *block_request(GraceObject *self, Env *env, const char *name,
                                   GraceObject **args, int nargs, Cont *k) {
    GraceBlock *blk = (GraceBlock *)self;
    if (strncmp(name, "apply(", 6) == 0) {
        GraceObject *scope = grace_user_new(blk->lex_scope);
        int pi = 0;
        for (ASTNode *p = blk->params; p != NULL && p->kind == NK_CONS; p = p->a2) {
            ASTNode *decl = p->a1; if (!decl) break;
            user_bind_def(scope, decl->strval, (pi < nargs) ? args[pi] : grace_done);
            pi++;
        }
        Env *inner = malloc(sizeof(Env));
        inner->refcount = 1;
        inner->receiver = blk->lex_self ? blk->lex_self : env->receiver;
        inner->scope    = scope;
        inner->return_k = cont_retain(blk->return_k ? blk->return_k : k);
        inner->except_k = cont_retain(env->except_k);
        inner->reset_k  = cont_retain(env->reset_k);
        BlockBodyStep *step = malloc(sizeof(BlockBodyStep));
        step->base.go = block_body_go;
        step->base.gc_trace = block_body_step_trace;
        step->body     = blk->body;
        step->inner_env = inner;
        step->k        = cont_retain(k);
        return (PendingStep *)step;
    }
    if (strcmp(name,"match(1)")==0) {
        if (list_length(blk->params) != 1)
            return cont_apply(k, make_match_result(0, grace_done));
        ASTNode *param = blk->params->a1;  /* first param: NK_IDENT_DECL */
        ASTNode *pat_list = param ? param->a1 : NULL;
        ASTNode *pat_expr = (pat_list && pat_list->kind == NK_CONS) ? pat_list->a1 : NULL;
        if (!pat_expr) {
            /* No pattern - catch-all, always matches. Apply and wrap. */
            BlockMatchWrapCont *wc = CONT_ALLOC(BlockMatchWrapCont);
            wc->base.apply = bmw_apply;
            wc->base.gc_trace = bmw_trace;
            wc->base.cleanup = bmw_cleanup;
            wc->k = cont_retain(k);
            GraceObject *a[1] = { args[0] };
            return block_request(self, env, "apply(1)", a, 1, (Cont *)wc);
        }
        /* Has pattern - evaluate it in block's lexical scope, then match */
        Env *lex_env = malloc(sizeof(Env));
        lex_env->refcount = 1;
        lex_env->receiver = blk->lex_self ? blk->lex_self : env->receiver;
        lex_env->scope    = blk->lex_scope;
        lex_env->return_k = cont_retain(env->return_k);
        lex_env->except_k = cont_retain(env->except_k);
        lex_env->reset_k  = cont_retain(env->reset_k);
        BlockMatchPatternCont *pc = CONT_ALLOC(BlockMatchPatternCont);
        pc->base.apply = bmp_apply;
        pc->base.gc_trace = bmp_trace;
        pc->base.cleanup = bmp_cleanup;
        pc->blk    = self;
        pc->target = args[0];
        pc->env    = env_retain(env);
        pc->k      = cont_retain(k);
        PendingStep *r = eval_node(pat_expr, lex_env, (Cont *)pc);
        env_release(lex_env);
        return r;
    }
    if (strcmp(name,"|(1)")==0)
        return cont_apply(k, grace_patternor_new(self, args[0]));
    if (strcmp(name,"asString(0)")==0) return cont_apply(k, grace_string_new("a block"));
    grace_fatal("No method '%s' on Block", name);
}
static const char *block_describe(GraceObject *self) { (void)self; return "a block"; }
static void block_trace(GraceObject *self) {
    GraceBlock *blk = (GraceBlock *)self;
    gc_mark_grey(blk->lex_scope);
    gc_mark_grey(blk->lex_self);
    gc_trace_cont(blk->return_k);
    gc_trace_cont(blk->except_k);
}
static void block_sweep_free(GraceObject *self) {
    GraceBlock *blk = (GraceBlock *)self;
    cont_release(blk->return_k);
    cont_release(blk->except_k);
}
const GraceVTable grace_block_vtable = { block_request, block_describe, block_trace, block_sweep_free };
GraceObject *grace_block_new(ASTNode *params, ASTNode *body, GraceObject *scope,
                              GraceObject *self_obj, Cont *return_k, Cont *except_k) {
    GraceBlock *blk = (GraceBlock *)gc_alloc(sizeof(GraceBlock));
    blk->base.vt  = &grace_block_vtable;
    blk->params   = params;
    blk->body     = body;
    blk->lex_scope = scope;
    blk->lex_self  = self_obj;
    blk->return_k  = cont_retain(return_k);
    blk->except_k  = cont_retain(except_k);
    return (GraceObject *)blk;
}

/*  GraceLineup  */

/* Iteration continuation */
typedef struct {
    int          idx;
    GraceObject *lineup;   /* keeps lineup alive during GC */
    GraceObject *block;
    Env         *env;
    Cont        *k;
} LineupIterState;
typedef struct { Cont base; LineupIterState *st; } LineupIterCont;

static void lineup_iter_trace(Cont *c) {
    LineupIterCont *lc = (LineupIterCont *)c;
    LineupIterState *st = lc->st;
    if (!st) return;
    gc_mark_grey(st->lineup);
    gc_mark_grey(st->block);
    gc_trace_env(st->env);
    gc_trace_cont(st->k);
}
static void lineup_iter_cleanup(Cont *c) {
    LineupIterCont *lc = (LineupIterCont *)c;
    LineupIterState *st = lc->st;
    if (!st) return;
    env_release(st->env);
    cont_release_abandon(st->k);
    free(st);
}

static PendingStep *lineup_iter_next(Cont *c, GraceObject *v) {
    (void)v;
    LineupIterCont *lc = (LineupIterCont *)c;
    LineupIterState *st = lc->st;
    st->idx++;
    GraceLineup *lu = (GraceLineup *)st->lineup;
    if (st->idx >= lu->n) {
        Cont *k = cont_retain(st->k);
        lc->st = NULL;
        cont_consumed(c);
        env_release(st->env);
        cont_release(st->k);
        free(st);
        PendingStep *r = cont_apply(k, grace_done);
        cont_release(k);
        return r;
    }
    GraceObject *a[1] = { lu->elems[st->idx] };
    LineupIterCont *nc = CONT_ALLOC(LineupIterCont);
    nc->base.apply    = lineup_iter_next;
    nc->base.gc_trace = lineup_iter_trace;
    nc->base.cleanup  = lineup_iter_cleanup;
    nc->st = st;
    lc->st = NULL;
    cont_consumed(c);
    return grace_request(st->block, st->env, "apply(1)", a, 1, (Cont *)nc);
}

/* asString continuation */
typedef struct {
    int          idx;       /* index of element whose asString we're waiting for */
    GraceObject *lineup;
    char        *so_far;
    Env         *env;
    Cont        *k;
    const char  *elem_method; /* "asString(0)" or "asDebugString(0)" */
} LineupAsStrState;
typedef struct { Cont base; LineupAsStrState *st; } LineupAsStrCont;

static void lineup_asstr_trace(Cont *c) {
    LineupAsStrCont *lc = (LineupAsStrCont *)c;
    LineupAsStrState *st = lc->st;
    if (!st) return;
    gc_mark_grey(st->lineup);
    gc_trace_env(st->env);
    gc_trace_cont(st->k);
}
static void lineup_asstr_cleanup(Cont *c) {
    LineupAsStrCont *lc = (LineupAsStrCont *)c;
    LineupAsStrState *st = lc->st;
    if (!st) return;
    env_release(st->env);
    cont_release_abandon(st->k);
    free(st->so_far);
    free(st);
}

static PendingStep *lineup_asstr_next(Cont *c, GraceObject *str_v) {
    LineupAsStrCont *lc = (LineupAsStrCont *)c;
    LineupAsStrState *st = lc->st;
    GraceLineup *lu = (GraceLineup *)st->lineup;
    const char *part = grace_string_val(str_v);
    const char *sep  = (st->idx == 0) ? "" : ", ";
    char *next_sf = str_fmt("%s%s%s", st->so_far, sep, part);
    free(st->so_far);
    st->so_far = next_sf;
    st->idx++;
    if (st->idx >= lu->n) {
        char *result = str_fmt("[%s]", st->so_far);
        Cont *k = cont_retain(st->k);
        lc->st = NULL;
        env_release(st->env);
        cont_release(st->k);
        free(st->so_far);
        free(st);
        cont_consumed(c);
        PendingStep *r = cont_apply(k, grace_string_take(result));
        cont_release(k);
        return r;
    }
    LineupAsStrCont *nc = CONT_ALLOC(LineupAsStrCont);
    nc->base.apply    = lineup_asstr_next;
    nc->base.gc_trace = lineup_asstr_trace;
    nc->base.cleanup  = lineup_asstr_cleanup;
    nc->st = st;
    lc->st = NULL;
    cont_consumed(c);
    return grace_request(lu->elems[st->idx], st->env, st->elem_method, NULL, 0, (Cont *)nc);
}

/* vtable dispatch */
static PendingStep *lineup_request(GraceObject *self, Env *env, const char *name,
                                    GraceObject **args, int nargs, Cont *k) {
    (void)nargs;
    GraceLineup *lu = (GraceLineup *)self;

    if (strcmp(name, "do(1)") == 0 || strcmp(name, "each(1)") == 0) {
        if (lu->n == 0) return cont_apply(k, grace_done);
        LineupIterState *st = malloc(sizeof(LineupIterState));
        st->idx    = 0;
        st->lineup = self;
        st->block  = args[0];
        st->env    = env_retain(env);
        st->k      = cont_retain(k);
        LineupIterCont *lc = CONT_ALLOC(LineupIterCont);
        lc->base.apply    = lineup_iter_next;
        lc->base.gc_trace = lineup_iter_trace;
        lc->base.cleanup  = lineup_iter_cleanup;
        lc->st = st;
        GraceObject *a[1] = { lu->elems[0] };
        return grace_request(args[0], env, "apply(1)", a, 1, (Cont *)lc);
    }

    if (strcmp(name, "size(0)") == 0)
        return cont_apply(k, grace_number_new((double)lu->n));

    if (strcmp(name, "at(1)") == 0) {
        int idx = (int)grace_number_val(args[0]) - 1;
        if (idx < 0 || idx >= lu->n)
            grace_raise(env, "BoundsError", "lineup index %d out of range (size %d)",
                        idx + 1, lu->n);
        return cont_apply(k, lu->elems[idx]);
    }

    if (strcmp(name, "++(1)") == 0) {
        GraceObject *other = args[0];
        if (other->vt != &grace_lineup_vtable)
            grace_raise(env, "TypeError", "++ requires a lineup argument");
        GraceLineup *b = (GraceLineup *)other;
        int n2 = lu->n + b->n;
        GraceObject **arr = malloc((size_t)(n2 > 0 ? n2 : 1) * sizeof(GraceObject *));
        for (int i = 0; i < lu->n; i++) arr[i]          = lu->elems[i];
        for (int i = 0; i < b->n;  i++) arr[lu->n + i]  = b->elems[i];
        return cont_apply(k, grace_lineup_new(arr, n2));
    }

    if (strcmp(name, "asString(0)") == 0 || strcmp(name, "asDebugString(0)") == 0) {
        if (lu->n == 0) return cont_apply(k, grace_string_new("[]"));
        const char *elem_meth = (strcmp(name, "asDebugString(0)") == 0)
                                ? "asDebugString(0)" : "asString(0)";
        LineupAsStrState *st = malloc(sizeof(LineupAsStrState));
        st->idx    = 0;
        st->lineup = self;
        st->so_far = str_dup("");
        st->env    = env_retain(env);
        st->k      = cont_retain(k);
        st->elem_method = elem_meth;
        LineupAsStrCont *lc = CONT_ALLOC(LineupAsStrCont);
        lc->base.apply    = lineup_asstr_next;
        lc->base.gc_trace = lineup_asstr_trace;
        lc->base.cleanup  = lineup_asstr_cleanup;
        lc->st = st;
        return grace_request(lu->elems[0], env, elem_meth, NULL, 0, (Cont *)lc);
    }

    if (strcmp(name, "==(1)") == 0)
        return cont_apply(k, grace_bool(self == args[0]));
    if (strcmp(name, "!=(1)") == 0)
        return cont_apply(k, grace_bool(self != args[0]));

    grace_fatal("No method '%s' on lineup", name);
}

static const char *lineup_describe(GraceObject *self) { (void)self; return "a lineup"; }

static void lineup_trace(GraceObject *self) {
    GraceLineup *lu = (GraceLineup *)self;
    for (int i = 0; i < lu->n; i++)
        gc_mark_grey(lu->elems[i]);
}

static void lineup_sweep_free(GraceObject *self) {
    GraceLineup *lu = (GraceLineup *)self;
    free(lu->elems);
    lu->elems = NULL;
}

const GraceVTable grace_lineup_vtable = { lineup_request, lineup_describe, lineup_trace, lineup_sweep_free };

GraceObject *grace_lineup_new(GraceObject **elems, int n) {
    GraceLineup *lu = (GraceLineup *)gc_alloc(sizeof(GraceLineup));
    lu->base.vt = &grace_lineup_vtable;
    lu->n       = n;
    lu->elems   = elems;   /* takes ownership of the malloc'd array */
    return (GraceObject *)lu;
}

/*  GracePrimitiveArray  (mutable fixed-size array) */

/* Iteration continuation structures for primitiveArray.do(1) */
typedef struct {
    int           idx;
    GracePrimitiveArray *arr;
    GraceObject  *block;
    Env          *env;
    Cont         *k;
} PrimArrayIterState;
typedef struct { Cont base; PrimArrayIterState *st; } PrimArrayIterCont;

static void primarray_iter_trace(Cont *c) {
    PrimArrayIterCont *pc = (PrimArrayIterCont *)c;
    PrimArrayIterState *st = pc->st;
    if (!st) return;
    gc_mark_grey((GraceObject *)st->arr);
    gc_mark_grey(st->block);
    gc_trace_env(st->env);
    gc_trace_cont(st->k);
}
static void primarray_iter_cleanup(Cont *c) {
    PrimArrayIterCont *pc = (PrimArrayIterCont *)c;
    PrimArrayIterState *st = pc->st;
    if (!st) return;
    env_release(st->env);
    cont_release_abandon(st->k);
    free(st);
}
static PendingStep *primarray_iter_next(Cont *c, GraceObject *v) {
    (void)v;
    PrimArrayIterCont *pc = (PrimArrayIterCont *)c;
    PrimArrayIterState *st = pc->st;
    st->idx++;
    if (st->idx >= st->arr->capacity) {
        Cont *k = cont_retain(st->k);
        pc->st = NULL;
        env_release(st->env);
        cont_release(st->k);
        free(st);
        cont_consumed(c);
        PendingStep *r = cont_apply(k, grace_done);
        cont_release(k);
        return r;
    }
    PrimArrayIterCont *nc = CONT_ALLOC(PrimArrayIterCont);
    nc->base.apply    = primarray_iter_next;
    nc->base.gc_trace = primarray_iter_trace;
    nc->base.cleanup  = primarray_iter_cleanup;
    nc->st = st;
    pc->st = NULL;
    cont_consumed(c);
    GraceObject *a[1] = { st->arr->elems[st->idx] };
    return grace_request(st->block, st->env, "apply(1)", a, 1, (Cont *)nc);
}

static PendingStep *primarray_request(GraceObject *self, Env *env,
                                       const char *name,
                                       GraceObject **args, int nargs, Cont *k) {
    (void)nargs;
    GracePrimitiveArray *pa = (GracePrimitiveArray *)self;
    if (strcmp(name, "at(1)") == 0) {
        int idx = (int)grace_number_val(args[0]);
        if (idx < 0 || idx >= pa->capacity)
            grace_raise(env, "BoundsError", "primitiveArray index %d out of bounds (size %d)",
                        idx, pa->capacity);
        if (pa->elems[idx] == grace_uninit)
            grace_raise(env, "UninitError", "primitiveArray element at %d uninitialised", idx);
        return cont_apply(k, pa->elems[idx]);
    }
    if (strcmp(name, "at(1)put(1)") == 0) {
        int idx = (int)grace_number_val(args[0]);
        if (idx < 0 || idx >= pa->capacity)
            grace_raise(env, "BoundsError", "primitiveArray index %d out of bounds (size %d)",
                        idx, pa->capacity);
        gc_write_barrier(args[1]);
        pa->elems[idx] = args[1];
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "size(0)") == 0)
        return cont_apply(k, grace_number_new((double)pa->capacity));
    if (strcmp(name, "do(1)") == 0 || strcmp(name, "each(1)") == 0) {
        if (pa->capacity == 0) return cont_apply(k, grace_done);
        PrimArrayIterState *st = malloc(sizeof(PrimArrayIterState));
        st->idx   = 0;
        st->arr   = pa;
        st->block = args[0];
        st->env   = env_retain(env);
        st->k     = cont_retain(k);
        PrimArrayIterCont *pc = CONT_ALLOC(PrimArrayIterCont);
        pc->base.apply    = primarray_iter_next;
        pc->base.gc_trace = primarray_iter_trace;
        pc->base.cleanup  = primarray_iter_cleanup;
        pc->st = st;
        GraceObject *a[1] = { pa->elems[0] };
        return grace_request(args[0], env, "apply(1)", a, 1, (Cont *)pc);
    }
    if (strcmp(name, "asString(0)") == 0)
        return cont_apply(k, grace_string_new("a primitiveArray"));
    grace_fatal("No method '%s' on primitiveArray", name);
}
static const char *primarray_describe(GraceObject *self) { (void)self; return "a primitiveArray"; }
static void primarray_trace(GraceObject *self) {
    GracePrimitiveArray *pa = (GracePrimitiveArray *)self;
    for (int i = 0; i < pa->capacity; i++)
        if (pa->elems[i]) gc_mark_grey(pa->elems[i]);
}
static void primarray_sweep_free(GraceObject *self) {
    GracePrimitiveArray *pa = (GracePrimitiveArray *)self;
    free(pa->elems);
    pa->elems = NULL;
}
const GraceVTable grace_primarray_vtable = { primarray_request, primarray_describe, primarray_trace, primarray_sweep_free };

GraceObject *grace_primarray_new(int capacity) {
    GracePrimitiveArray *pa = (GracePrimitiveArray *)gc_alloc(sizeof(GracePrimitiveArray));
    pa->base.vt   = &grace_primarray_vtable;
    pa->capacity   = capacity;
    pa->elems     = malloc((size_t)(capacity > 0 ? capacity : 1) * sizeof(GraceObject *));
    for (int i = 0; i < capacity; i++) pa->elems[i] = grace_uninit;
    return (GraceObject *)pa;
}

/*  GraceUserObject  */
static PendingStep *user_request(GraceObject *self, Env *env, const char *name,
                                  GraceObject **args, int nargs, Cont *k) {
    GraceUserObject *uo = (GraceUserObject *)self;
    /* Check own methods */
    for (MethodEntry *m = uo->methods; m; m = m->next)
        if (strcmp(m->name, name) == 0)
            return m->fn(self, env, args, nargs, k, m->data);
    /* Walk lexical parent chain */
    for (GraceObject *c = uo->lex_parent; c; ) {
        if (c->vt != &grace_user_vtable) break;
        GraceUserObject *pu = (GraceUserObject *)c;
        for (MethodEntry *m = pu->methods; m; m = m->next)
            if (strcmp(m->name, name) == 0)
                return m->fn(c, env, args, nargs, k, m->data);
        c = pu->lex_parent;
    }
    if (uo->dialect)
        return grace_request(uo->dialect, env, name, args, nargs, k);
    /* Default identity equality/inequality available on all objects */
    if (strcmp(name, "==(1)") == 0)
        return cont_apply(k, grace_bool(self == args[0]));
    if (strcmp(name, "!=(1)") == 0)
        return cont_apply(k, grace_bool(self != args[0]));
    if (strcmp(name, "asString(0)") == 0)
        return cont_apply(k, grace_string_new("an object"));
    if (strcmp(name, "asDebugString(0)") == 0) {
        /* Build "object { method name1; method name2; ... }" */
        char *buf = str_dup("object {");
        for (MethodEntry *m = uo->methods; m; m = m->next) {
            /* Skip duplicates (methods are hoisted then re-added) */
            int dup = 0;
            for (MethodEntry *prev = uo->methods; prev != m; prev = prev->next)
                if (strcmp(prev->name, m->name) == 0) { dup = 1; break; }
            if (dup) continue;
            char *next = str_fmt("%s method %s;", buf, m->name);
            free(buf);
            buf = next;
        }
        char *result = str_fmt("%s }", buf);
        free(buf);
        return cont_apply(k, grace_string_take(result));
    }
    if (strcmp(name, "hash(0)") == 0)
        return cont_apply(k, grace_number_new((double)(uint32_t)(intptr_t)self));
    grace_fatal("No method '%s' on object", name);
}
static const char *user_describe(GraceObject *self) { (void)self; return "an object"; }

static void user_trace(GraceObject *self) {
    GraceUserObject *uo = (GraceUserObject *)self;
    gc_mark_grey(uo->lex_parent);
    gc_mark_grey(uo->dialect);
    /* Trace GraceObject* refs inside method entries */
    for (MethodEntry *m = uo->methods; m; m = m->next) {
        if (m->trace_data && m->data)
            m->trace_data(m->data);
    }
}

static void user_sweep_free(GraceObject *self) {
    GraceUserObject *uo = (GraceUserObject *)self;
    MethodEntry *m = uo->methods;
    uo->methods = NULL;
    uo->lex_parent = NULL;
    while (m) {
        MethodEntry *next = m->next;
        if (m->free_data && m->data)
            m->free_data(m->data);
        free((void *)m->name);
        free(m);
        m = next;
    }
}

const GraceVTable grace_user_vtable = { user_request, user_describe, user_trace, user_sweep_free };

GraceObject *grace_user_new(GraceObject *lex_parent) {
    GraceUserObject *obj = (GraceUserObject *)gc_alloc(sizeof(GraceUserObject));
    obj->base.vt    = &grace_user_vtable;
    obj->lex_parent = lex_parent;
    return (GraceObject *)obj;
}

void user_add_method(GraceObject *obj, const char *name, MethodFn fn, void *data) {
    if (obj->vt != &grace_user_vtable)
        grace_fatal("user_add_method on non-user object");
    GraceUserObject *uo = (GraceUserObject *)obj;
    MethodEntry *e = malloc(sizeof(MethodEntry));
    e->name = str_dup(name); e->fn = fn; e->data = data; e->next = NULL;
    e->trace_data = NULL; e->free_data = NULL;
    if (!uo->methods) {
        uo->methods = e;
    } else {
        MethodEntry *last = uo->methods;
        while (last->next) last = last->next;
        last->next = e;
    }
}

static PendingStep *def_fn(GraceObject *self, Env *env, GraceObject **args,
                            int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)args;(void)nargs;
    return cont_apply(k, (GraceObject *)data);
}

static void def_trace_data(void *data) {
    gc_mark_grey((GraceObject *)data);
}

void user_bind_def(GraceObject *obj, const char *name, GraceObject *value) {
    gc_write_barrier(value);  /* value may be WHITE; obj may be BLACK (new frame) */
    /* If a def binding for this name already exists, update it in place.
     * This supports multi-shot continuations that re-bind the same def. */
    char *full = str_fmt("%s(0)", name);
    GraceUserObject *uo = (GraceUserObject *)obj;
    for (MethodEntry *m = uo->methods; m; m = m->next) {
        if (m->fn == def_fn && strcmp(m->name, full) == 0) {
            m->data = value;
            free(full);
            return;
        }
    }
    user_add_method(obj, full, def_fn, value);
    /* Set trace_data on the just-added method entry */
    MethodEntry *last = uo->methods;
    while (last->next) last = last->next;
    last->trace_data = def_trace_data;
    /* free_data = NULL: data is a GC-managed GraceObject*, don't free it */
    free(full);
}

typedef struct { GraceObject **cell; const char *name; } VarCell;

static PendingStep *var_getter_fn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;
    VarCell *vc = (VarCell *)data;
    if (*vc->cell == grace_uninit)
        grace_raise(env, "UninitliasedVariable", "Variable '%s' used before initialisation", vc->name);
    return cont_apply(k, *vc->cell);
}
static PendingStep *var_setter_fn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)nargs;
    VarCell *vc = (VarCell *)data;
    gc_write_barrier(args[0]);  /* write barrier: grey new value during marking */
    *vc->cell = args[0];
    return cont_apply(k, grace_done);
}

static void var_trace_data(void *data) {
    VarCell *vc = (VarCell *)data;
    if (vc->cell && *vc->cell)
        gc_mark_grey(*vc->cell);
}
static void var_free_data(void *data) {
    VarCell *vc = (VarCell *)data;
    free(vc->cell);
    free(vc);
}

void user_bind_var(GraceObject *obj, const char *name, GraceObject **cell) {
    VarCell *vc = malloc(sizeof(VarCell));
    vc->cell = cell;
    vc->name = name;
    char *getter_name = str_fmt("%s(0)",    name);
    char *setter_name = str_fmt("%s:=(1)",  name);
    user_add_method(obj, getter_name, var_getter_fn, vc);
    user_add_method(obj, setter_name, var_setter_fn, vc);
    free(getter_name);
    free(setter_name);
    /* Set trace/free on getter and setter entries */
    GraceUserObject *uo = (GraceUserObject *)obj;
    MethodEntry *last = uo->methods;
    while (last->next) last = last->next;
    /* last = setter, last's prev = getter */
    last->trace_data = var_trace_data;
    last->free_data  = NULL;  /* setter doesn't own the VarCell */
    /* Find getter (second to last) */
    MethodEntry *prev_last = uo->methods;
    if (prev_last != last) {
        while (prev_last->next != last) prev_last = prev_last->next;
        prev_last->trace_data = var_trace_data;
        prev_last->free_data  = var_free_data;  /* getter owns the VarCell */
    }
}

GraceObject *scope_find_receiver(GraceObject *scope, const char *name) {
    for (GraceObject *c = scope; c; ) {
        if (c->vt != &grace_user_vtable) break;
        GraceUserObject *uo = (GraceUserObject *)c;
        for (MethodEntry *m = uo->methods; m; m = m->next)
            if (strcmp(m->name, name) == 0) return c;
        c = uo->lex_parent;
    }
    return NULL;
}

/*  Exception prototype (for raise/refine)  */
typedef struct { char *tag; } ExcProtoData;

static GraceObject *make_exception_proto(const char *name);   /* forward */

static PendingStep *excproto_raise_fn(GraceObject *self, Env *env, GraceObject **args,
                                       int nargs, Cont *k, void *data) {
    (void)self;(void)nargs;(void)k;
    grace_raise(env, ((ExcProtoData*)data)->tag, "%s", grace_string_val(args[0]));
    return NULL; /* unreachable */
}
static PendingStep *excproto_refine_fn(GraceObject *self, Env *env, GraceObject **args,
                                        int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)nargs;(void)data;
    return cont_apply(k, make_exception_proto(grace_string_val(args[0])));
}
static PendingStep *excproto_name_fn(GraceObject *self, Env *env, GraceObject **args,
                                      int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)args;(void)nargs;
    return cont_apply(k, grace_string_new(((ExcProtoData*)data)->tag));
}
static PendingStep *excproto_match_fn(GraceObject *self, Env *env, GraceObject **args,
                                       int nargs, Cont *k, void *data) {
    (void)self;(void)env;(void)nargs;
    const char *tag = ((ExcProtoData*)data)->tag;
    GraceObject *target = args[0];
    if (target->vt == &grace_exception_vtable) {
        GraceException *ex = (GraceException*)target;
        return cont_apply(k, make_match_result(ex->tag && strcmp(ex->tag, tag) == 0, target));
    }
    return cont_apply(k, make_match_result(0, grace_done));
}

static void excproto_free_data(void *data) {
    ExcProtoData *d = (ExcProtoData *)data;
    free(d->tag);
    free(d);
}

static GraceObject *make_exception_proto(const char *name) {
    GraceObject *p = grace_user_new(NULL);
    ExcProtoData *d = malloc(sizeof(ExcProtoData));
    d->tag = str_dup(name);
    user_add_method(p, "raise(1)",    excproto_raise_fn,  d);
    user_add_method(p, "refine(1)",   excproto_refine_fn, d);
    user_add_method(p, "name(0)",     excproto_name_fn,   d);
    user_add_method(p, "asString(0)", excproto_name_fn,   d);
    user_add_method(p, "match(1)",    excproto_match_fn,  d);
    /* Only the first method entry owns the ExcProtoData for freeing */
    GraceUserObject *uo = (GraceUserObject *)p;
    for (MethodEntry *m = uo->methods; m; m = m->next) {
        if (m->data == d && m->fn == excproto_raise_fn) {
            m->free_data = excproto_free_data;
            break;
        }
    }
    return p;
}
GraceObject *grace_exception_proto_new(const char *name) {
    return make_exception_proto(name);
}

/*  GraceException instances  */
static PendingStep *exception_request(GraceObject *self, Env *env, const char *name,
                                       GraceObject **args, int nargs, Cont *k) {
    (void)nargs;
    GraceException *ex = (GraceException*)self;
    if (strcmp(name,"message(0)")==0 || strcmp(name,"messageText(0)")==0)
        return cont_apply(k, grace_string_new(ex->message));
    if (strcmp(name,"asString(0)")==0)
        return cont_apply(k, grace_string_take(str_fmt("%s: %s",
                ex->tag ? ex->tag : "Error", ex->message)));
    if (strcmp(name,"raise(1)")==0) {
        grace_raise(env, ex->tag, "%s", grace_string_val(args[0]));
        return NULL;
    }
    if (strcmp(name,"refine(1)")==0)
        return cont_apply(k, make_exception_proto(grace_string_val(args[0])));
    grace_fatal("No method '%s' on Exception", name);
}
static const char *exception_describe(GraceObject *self) {
    static char buf[128];
    snprintf(buf, sizeof(buf), "Exception(%s)", ((GraceException*)self)->tag);
    return buf;
}
static void exception_sweep_free(GraceObject *self) {
    GraceException *ex = (GraceException *)self;
    if (ex->tag) free(ex->tag);
    if (ex->message) free(ex->message);
}
const GraceVTable grace_exception_vtable = { exception_request, exception_describe, NULL, exception_sweep_free };
GraceObject *grace_exception_new(const char *tag, const char *msg) {
    GraceException *ex = (GraceException *)gc_alloc(sizeof(GraceException));
    ex->base.vt = &grace_exception_vtable;
    ex->tag     = tag ? str_dup(tag) : str_dup("Error");
    ex->message = msg ? str_dup(msg) : str_dup("");
    return (GraceObject *)ex;
}
