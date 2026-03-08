/* grace.c - AST node constructors (implements the prototypes in ast.h) and
 * string constants used by the baked Grace source files.
 * Also provides the trampoline, env helpers, and general utility functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "ast.h"
#define GRACE_C
int log_requests = 0;
#include "grace.h"
#include "gc.h"

/* 
 * Special character string constants
 *  */
const char *c9Q = "\"";
const char *c9B = "\\";
const char *c9N = "\n";
const char *c9R = "\r";
const char *c9S = "*";
const char *c9M = "&";
const char *c9E = "!";
const char *c9P = "%";
const char *c9A = "@";
const char *c9L = "{";
const char *c9D = "$";
const char *c9G = "`";
const char *c9T = "~";
const char *c9C = "^";
const char *c9H = "#";

/* 
 * nil sentinel
 *  */
ASTNode *nil = NULL;

/* 
 * Utility helpers
 *  */
char *str_dup(const char *s) {
    if (!s) return NULL;
    size_t n = strlen(s) + 1;
    char *d = malloc(n);
    memcpy(d, s, n);
    return d;
}

char *str_cat(const char *a, const char *b) {
    if (!a) a = "";
    if (!b) b = "";
    size_t la = strlen(a), lb = strlen(b);
    char *r = malloc(la + lb + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb + 1);
    return r;
}

char *str_fmt(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *buf = malloc(n + 1);
    va_start(ap, fmt);
    vsnprintf(buf, n + 1, fmt, ap);
    va_end(ap);
    return buf;
}

int list_length(ASTNode *list) {
    int n = 0;
    for (ASTNode *p = list; p != NULL; p = p->a2) n++;
    return n;
}

/* 
 * s4F - three-way string concatenation
 *  */
const char *s4F(const char *a, const char *b, const char *c) {
    if (!a) a = "";
    if (!b) b = "";
    if (!c) c = "";
    size_t la = strlen(a), lb = strlen(b), lc = strlen(c);
    char *r = malloc(la + lb + lc + 1);
    memcpy(r, a, la);
    memcpy(r + la, b, lb);
    memcpy(r + la + lb, c, lc + 1);
    return r;
}

/* 
 * AST node allocator
 *  */
static ASTNode *ast_alloc(int kind) {
    ASTNode *n = calloc(1, sizeof(ASTNode));
    n->kind = kind;
    return n;
}

/* 
 * List constructors
 *  */
static ASTNode *cons_node(ASTNode *head, ASTNode *tail) {
    ASTNode *n = ast_alloc(NK_CONS);
    n->a1 = head; n->a2 = tail;
    return n;
}

ASTNode *_o1N_node(ASTNode *x) { return cons_node(x, NULL); }
ASTNode *_o1N_str(const char *s) { return cons_node(s0L(s), NULL); }
ASTNode *_c0N_node(ASTNode *head, ASTNode *tail) { return cons_node(head, tail); }
ASTNode *_c0N_str(const char *head, ASTNode *tail) { return cons_node(s0L(head), tail); }
ASTNode *c2N(ASTNode *a, ASTNode *b) { return cons_node(a, cons_node(b, NULL)); }

/* 
 * Node constructors
 *  */

ASTNode *o0C(ASTNode *body, ASTNode *anns) {
    ASTNode *n = ast_alloc(NK_OBJCONS);
    n->a1 = body; n->a2 = anns; return n;
}
ASTNode *n0M(double val) {
    ASTNode *n = ast_alloc(NK_NUMLIT);
    n->numval = val; return n;
}
ASTNode *s0L(const char *val) {
    ASTNode *n = ast_alloc(NK_STRLIT);
    n->strval = val; return n;
}
ASTNode *b1K(ASTNode *params, ASTNode *stmts) {
    ASTNode *n = ast_alloc(NK_BLOCK);
    n->a1 = params; n->a2 = stmts; return n;
}
ASTNode *d3F(const char *name, ASTNode *dtype, ASTNode *anns, ASTNode *val) {
    ASTNode *n = ast_alloc(NK_DEF_DECL);
    n->strval = name; n->a1 = dtype; n->a2 = anns; n->a3 = val; return n;
}
ASTNode *v4R(const char *name, ASTNode *dtype, ASTNode *anns, ASTNode *val) {
    ASTNode *n = ast_alloc(NK_VAR_DECL);
    n->strval = name; n->a1 = dtype; n->a2 = anns; n->a3 = val; return n;
}
ASTNode *m0D(ASTNode *parts, ASTNode *rettype, ASTNode *anns, ASTNode *body) {
    ASTNode *n = ast_alloc(NK_METH_DECL);
    n->a1 = parts; n->a2 = rettype; n->a3 = anns; n->a4 = body; return n;
}
ASTNode *p0T(const char *name, ASTNode *params, ASTNode *gen) {
    ASTNode *n = ast_alloc(NK_PART);
    n->strval = name; n->a1 = params; n->a2 = gen; return n;
}
/* l0R - lexical request: strval=name-with-arity, a1=flat-args, a2=generics */
ASTNode *l0R(const char *name, ASTNode *args, ASTNode *gen) {
    ASTNode *n = ast_alloc(NK_LEXREQ);
    n->strval = name; n->a1 = args; n->a2 = gen; return n;
}
/* d0R - explicit/dot request: strval=name, a1=receiver, a2=flat-args, a3=generics */
ASTNode *d0R(ASTNode *recv, const char *name, ASTNode *args, ASTNode *gen) {
    ASTNode *n = ast_alloc(NK_DOTREQ);
    n->strval = name; n->a1 = recv; n->a2 = args; n->a3 = gen; return n;
}
ASTNode *a5N(ASTNode *left, ASTNode *right) {
    ASTNode *n = ast_alloc(NK_ASSIGN);
    n->a1 = left; n->a2 = right; return n;
}
ASTNode *r3T(ASTNode *val) {
    ASTNode *n = ast_alloc(NK_RETURN_STMT);
    n->a1 = val; return n;
}
ASTNode *i0D(const char *name, ASTNode *dtype) {
    ASTNode *n = ast_alloc(NK_IDENT_DECL);
    n->strval = name; n->a1 = dtype; return n;
}
ASTNode *i0S(const char *pre, ASTNode *expr, ASTNode *rest) {
    ASTNode *n = ast_alloc(NK_INTERP_STR);
    n->strval = pre; n->a1 = expr; n->a2 = rest; return n;
}
ASTNode *i0M(const char *src, ASTNode *binding) {
    ASTNode *n = ast_alloc(NK_IMPORT_STMT);
    n->strval = src; n->a1 = binding; return n;
}
ASTNode *d0S(const char *src) {
    ASTNode *n = ast_alloc(NK_DIALECT_STMT);
    n->strval = src; return n;
}
ASTNode *c0M(const char *text) {
    ASTNode *n = ast_alloc(NK_COMMENT);
    n->strval = text; return n;
}
ASTNode *i0C(ASTNode *body) {
    ASTNode *n = ast_alloc(NK_IFACE_CONS);
    n->a1 = body; return n;
}
ASTNode *m0S(ASTNode *parts, ASTNode *rettype) {
    ASTNode *n = ast_alloc(NK_METH_SIG);
    n->a1 = parts; n->a2 = rettype; return n;
}
ASTNode *t0D(const char *name, ASTNode *gen, ASTNode *val) {
    ASTNode *n = ast_alloc(NK_TYPE_DECL);
    n->strval = name; n->a1 = gen; n->a2 = val; return n;
}

/* 
 * Trampoline
 *  */

/* Global pointer to the current pending step - used as a GC root. */
static PendingStep *gc_current_step = NULL;

/* Root tracer for the current trampoline step */
static void trace_current_step(void) {
    PendingStep *step = gc_current_step;
    if (step && step->gc_trace)
        step->gc_trace(step);
}

static int gc_step_tracer_registered = 0;

void trampoline(PendingStep *step) {
    if (!gc_step_tracer_registered) {
        gc_register_root_tracer(trace_current_step);
        gc_step_tracer_registered = 1;
    }
    gc_trampoline_enter();
    while (step) {
        step = step->go(step);
        gc_current_step = step;  /* update to new step before potential GC */
        gc_maybe_collect();
    }
    gc_current_step = NULL;
    gc_trampoline_exit();
}

/* 
 * cont_done - terminal continuation
 *  */
static PendingStep *cont_done_apply(Cont *self, GraceObject *value) {
    (void)self; (void)value;
    return NULL;
}
static Cont _cont_done = { .apply = cont_done_apply, .gc_trace = NULL, .cleanup = NULL, .refcount = CONT_REFCOUNT_STATIC };
Cont *cont_done = &_cont_done;

/* 
 * Environment helpers
 *  */
Env *env_extend(const Env *env, GraceObject *new_scope) {
    Env *e = malloc(sizeof(Env));
    *e = *env;
    e->refcount = 1;
    e->scope = new_scope;
    cont_retain(e->return_k);
    cont_retain(e->except_k);
    return e;
}

static void trace_module_registry(void);  /* forward declaration */

Env *env_new(GraceObject *prelude) {
    static int gc_initialized = 0;
    if (!gc_initialized) {
        gc_register_root_tracer(trace_module_registry);
        gc_initialized = 1;
    }
    Env *e = malloc(sizeof(Env));
    e->refcount = 1;
    e->receiver = prelude;
    e->scope    = prelude;
    e->return_k = cont_retain(cont_done);
    e->except_k = cont_retain(cont_done);
    return e;
}

/* 
 * Error handling
 *  */
void grace_fatal(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "Grace fatal error: ");
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

void grace_raise(Env *env, const char *tag, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    char buf[2048];
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);
    if (env && env->except_k && env->except_k != cont_done) {
        GraceObject *ex = grace_exception_new(tag, buf);
        trampoline(cont_apply(env->except_k, ex));
    } else {
        fprintf(stderr, "%s: %s\n", tag ? tag : "Error", buf);
        exit(1);
    }
}

/* 
 * grace_raise_step - return a PendingStep that raises an exception
 *  */
typedef struct {
    PendingStep  base;
    Env         *env;
    char        *tag;
    char        *msg;
} RaiseStep;

static PendingStep *raise_step_go(PendingStep *self) {
    RaiseStep *rs = (RaiseStep *)self;
    grace_raise(rs->env, rs->tag, "%s", rs->msg);
    return NULL; /* grace_raise may not return */
}

static void raise_step_trace(PendingStep *self) {
    RaiseStep *rs = (RaiseStep *)self;
    gc_trace_env(rs->env);
}

PendingStep *grace_raise_step(Env *env, const char *tag,
                               const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    char *buf = malloc(n + 1);
    va_start(ap, fmt);
    vsnprintf(buf, n + 1, fmt, ap);
    va_end(ap);
    RaiseStep *rs = malloc(sizeof(RaiseStep));
    rs->base.go = raise_step_go;
    rs->base.gc_trace = raise_step_trace;
    rs->env = env;
    rs->tag = str_dup(tag ? tag : "Error");
    rs->msg = buf;
    return (PendingStep *)rs;
}

/* 
 * Module registry
 *  */
typedef struct ModEntry { const char *name; GraceObject *mod; struct ModEntry *next; } ModEntry;
static ModEntry *mod_registry = NULL;

void grace_register_module(const char *name, GraceObject *module) {
    ModEntry *e = malloc(sizeof(ModEntry));
    e->name = name; e->mod = module; e->next = mod_registry;
    mod_registry = e;
}

GraceObject *grace_find_module(const char *name) {
    for (ModEntry *e = mod_registry; e; e = e->next)
        if (strcmp(e->name, name) == 0) return e->mod;
    return NULL;
}

/* GC root tracer for the module registry */
static void trace_module_registry(void) {
    for (ModEntry *e = mod_registry; e; e = e->next)
        gc_mark_grey(e->mod);
    /* Also trace static singletons' children (they are GC_STATIC so won't
     * be marked grey themselves, but we still need to reach objects
     * reachable from them if any were ever added). */
    /* grace_done, grace_uninit have no GraceObject* children */
    /* grace_true, grace_false have no GraceObject* children */
}

/* 
 * Synchronous request helper - runs trampoline internally
 *  */
PendingStep *capture_apply(Cont *self, GraceObject *v) {
    ((CaptureCont *)self)->result = v;
    return NULL;
}

static void capture_cont_trace(Cont *self) {
    CaptureCont *cc = (CaptureCont *)self;
    gc_mark_grey(cc->result);
}

GraceObject *grace_request_sync(GraceObject *recv, Env *env,
                                 const char *name,
                                 GraceObject **args, int nargs) {
    CaptureCont *k = CONT_ALLOC(CaptureCont);
    k->base.apply = capture_apply;
    k->base.gc_trace = capture_cont_trace;
    k->base.cleanup = NULL;
    k->result = NULL;
    trampoline(grace_request(recv, env, name, args, nargs, (Cont *)k));
    GraceObject *result = k->result;
    cont_release((Cont *)k);
    return result;
}