/* grace.h - Shared types and forward declarations for the Grace CPS interpreter. */

#ifndef GRACE_H
#define GRACE_H

#include <stddef.h>
#include <stdlib.h>
#include "ast.h"

#ifndef GRACE_C
extern int log_requests;
#endif
/* 
 * Trampoline / pending step
 *  */
typedef struct PendingStep PendingStep;
typedef PendingStep *(*StepFn)(PendingStep *self);

struct PendingStep {
    StepFn go;
    /* Optional GC trace function: marks all GraceObject* reachable from this step.
     * May be NULL if the step has no GraceObject* references. */
    void (*gc_trace)(PendingStep *self);
    /* Concrete step structs embed base as first member and add payload. */
};

/* Run the trampoline: loops until step == NULL. */
void trampoline(PendingStep *step);

/* 
 * Continuation
 *  */
typedef struct GraceObject GraceObject;
typedef struct Cont Cont;

typedef PendingStep *(*ContFn)(Cont *self, GraceObject *value);

struct Cont {
    ContFn apply;
    /* Optional GC trace function: marks all GraceObject* reachable from this cont
     * and recursively traces any inner continuations. May be NULL. */
    void (*gc_trace)(Cont *self);
    /* Optional cleanup: release retained internal refs (env, k) before free.
     * Called exactly once, when the cont's refcount drops to 0. */
    void (*cleanup)(Cont *self);
    /* GC epoch: used to avoid re-tracing the same continuation twice in one
     * GC cycle.  Set to gc_trace_epoch when first traced; gc_trace_cont
     * skips conts whose epoch matches the current collection. */
    unsigned int gc_epoch;
    /* Reference count for shared ownership.  Starts at 1.
     * Static/stack conts use a high sentinel so they are never freed. */
    int refcount;
    /* Set to 1 after the first cont_consumed call.  Prevents double-decrement
     * when a continuation is re-invoked via non-local return. */
    int consumed;
    /* Doubly-linked list for GC sweep of abandoned continuations. */
    Cont *gc_cont_prev;
    Cont *gc_cont_next;
    /* Concrete cont structs embed base as first member and add payload. */
};

/* Call a continuation with a value (shorthand). */
static inline PendingStep *cont_apply(Cont *k, GraceObject *v) {
    return k->apply(k, v);
}

#define CONT_REFCOUNT_STATIC 0x7FFFFF00  /* sentinel: never freed */

/* Global doubly-linked list of all heap-allocated continuations.
 * Used by the GC to sweep abandoned conts that refcounting alone cannot free
 * (e.g. when non-local returns or shift bypass intermediate continuations). */
extern Cont gc_cont_sentinel;
void gc_cont_unlink(Cont *k);

/* Allocate a zero'd continuation of the given type with refcount = 1,
 * and link it into the global cont list for GC sweep. */
#define CONT_ALLOC(type) ({ \
    type *_c = calloc(1, sizeof(type)); \
    _c->base.refcount = 1; \
    _c->base.gc_epoch = gc_current_epoch(); \
    _c->base.gc_cont_next = gc_cont_sentinel.gc_cont_next; \
    _c->base.gc_cont_prev = &gc_cont_sentinel; \
    gc_cont_sentinel.gc_cont_next->gc_cont_prev = (Cont *)_c; \
    gc_cont_sentinel.gc_cont_next = (Cont *)_c; \
    _c; \
})

static inline Cont *cont_retain(Cont *k) {
    if (k && k->refcount < CONT_REFCOUNT_STATIC) k->refcount++;
    return k;
}
static inline void cont_release(Cont *k) {
    if (k && k->refcount < CONT_REFCOUNT_STATIC && --k->refcount <= 0) {
        gc_cont_unlink(k);
        if (k->cleanup) k->cleanup(k);
        free(k);
    }
}
/* Mark a continuation as consumed (applied) and release the creator's reference.
 * Idempotent: a second call (for re-entrant continuations via non-local return)
 * is a no-op, keeping the cont alive for future re-invocations. */
static inline void cont_consumed(Cont *k) {
    if (!k || k->refcount >= CONT_REFCOUNT_STATIC) return;
    if (k->consumed) return;   /* re-entrant: don't double-decrement */
    k->consumed = 1;
    if (--k->refcount <= 0) {
        gc_cont_unlink(k);
        if (k->cleanup) k->cleanup(k);
        free(k);
    }
}

/* Release a continuation that may have been abandoned (never applied).
 * If the cont was never consumed, first consumes it (to remove the
 * creator ref), then releases the holder's ref.  Safe for normally-
 * consumed conts: cont_consumed is a no-op when already consumed.
 * If cont_consumed already freed the cont (rc was 1), we stop. */
static inline void cont_release_abandon(Cont *k) {
    if (!k || k->refcount >= CONT_REFCOUNT_STATIC) return;
    if (!k->consumed) {
        /* Not yet consumed: provide the missing consume decrement.
         * If this frees the cont (rc was 1), we're done. */
        k->consumed = 1;
        if (--k->refcount <= 0) {
            gc_cont_unlink(k);
            if (k->cleanup) k->cleanup(k);
            free(k);
            return;
        }
    }
    /* Now release the holder's reference. */
    cont_release(k);
}

/* A trivial "done" continuation: discards its argument and terminates. */
extern Cont *cont_done;

/* 
 * GraceObject and its vtable
 *  */
typedef struct Env Env;

typedef struct {
    /* CPS method request.
     * name    - method name including arity, e.g. "print(1)", "+(1)", "x(0)"
     * args    - flat C array of argument GraceObject* (all parts concatenated)
     * nargs   - total argument count
     * k       - continuation to call with the result
     */
    PendingStep *(*request)(GraceObject *self, Env *env, const char *name,
                            GraceObject **args, int nargs, Cont *k);

    /* Synchronous description for error messages (may return static buffer). */
    const char *(*describe)(GraceObject *self);

    /* GC: trace all GraceObject* children (call gc_mark_grey on each). */
    void (*trace)(GraceObject *self);

    /* GC: free ancillary (non-GC) heap data before the object is freed. */
    void (*sweep_free)(GraceObject *self);
} GraceVTable;

struct GraceObject {
    const GraceVTable *vt;
    struct GraceObject *gc_next;       /* GC heap chain */
    struct GraceObject *gc_grey_next;  /* GC grey-list chain */
    unsigned char       gc_color;      /* GC_WHITE/GREY/BLACK/STATIC */
    unsigned int        gc_epoch;      /* cycle in which this object was last marked */
};

/* Convenience: dispatch a method request. */
static inline PendingStep *grace_request(GraceObject *recv, Env *env,
                                          const char *name,
                                          GraceObject **args, int nargs,
                                          Cont *k) {
    // if (log_requests) {
    //     printf("grace_request: name=%s\n", name);
    // }
    return recv->vt->request(recv, env, name, args, nargs, k);
}

/* 
 * Execution environment
 *  */
struct Env {
    int          refcount;
    GraceObject *receiver;   /* self / current object */
    GraceObject *scope;      /* current lexical scope (a GraceUserObject chain) */
    Cont        *return_k;   /* continuation for `return expr` */
    Cont        *except_k;   /* continuation for exceptions */
    Cont        *reset_k;    /* innermost reset (delimited continuation) prompt */
};

static inline Env *env_retain(Env *e) { if (e) e->refcount++; return e; }
static inline void env_release(Env *e) {
    if (e && --e->refcount <= 0) {
        cont_release(e->return_k);
        cont_release(e->except_k);
        cont_release(e->reset_k);
        free(e);
    }
}

/* Create a copy of env with a new scope (does not mutate original). */
Env *env_extend(const Env *env, GraceObject *new_scope);

/* Create the initial environment containing the prelude scope. */
Env *env_new(GraceObject *prelude);

/* 
 * Special singletons
 *  */
extern GraceObject *grace_done;        /* returned from methods with no explicit return */
extern GraceObject *grace_uninit;      /* uninitialized variable sentinel */
extern GraceObject *grace_true;
extern GraceObject *grace_false;

/* Convenience: wrap bool. */
static inline GraceObject *grace_bool(int b) { return b ? grace_true : grace_false; }

/* 
 * GraceNumber
 *  */
typedef struct { GraceObject base; double value; } GraceNumber;
extern const GraceVTable grace_number_vtable;
GraceObject *grace_number_new(double v);
double grace_number_val(GraceObject *o);  /* asserts it's a number */

/* 
 * GraceString
 *  */
typedef struct { GraceObject base; const char *value; } GraceString;
extern const GraceVTable grace_string_vtable;
GraceObject *grace_string_new(const char *s);
GraceObject *grace_string_take(char *s);  /* takes ownership of malloc'd s */
const char *grace_string_val(GraceObject *o);   /* asserts it's a string */
GraceObject *grace_string_concat(GraceObject *a, GraceObject *b);

/* 
 * GraceBool
 *  */
typedef struct { GraceObject base; int value; } GraceBool;
extern const GraceVTable grace_bool_vtable;
int grace_bool_val(GraceObject *o);   /* asserts it's a bool */

/* 
 * GraceBlock
 *  */
typedef struct {
    GraceObject  base;
    ASTNode     *params;     /* NK_CONS list of NK_IDENT_DECL nodes */
    ASTNode     *body;       /* NK_CONS list of statement nodes */
    GraceObject *lex_scope;  /* captured lexical scope at block-creation time */
    GraceObject *lex_self;   /* captured self at block-creation time */
    Cont        *return_k;   /* enclosing method's return continuation */
    Cont        *except_k;   /* enclosing exception handler */
} GraceBlock;
extern const GraceVTable grace_block_vtable;
GraceObject *grace_block_new(ASTNode *params, ASTNode *body,
                              GraceObject *scope, GraceObject *self_obj,
                              Cont *return_k, Cont *except_k);

/*
 * GraceLineup  (immutable sequence of GraceObject*)
 *  */
typedef struct {
    GraceObject  base;
    int          n;
    GraceObject **elems;  /* malloc'd array of n elements */
} GraceLineup;
extern const GraceVTable grace_lineup_vtable;
GraceObject *grace_lineup_new(GraceObject **elems, int n);

/* 
 * GraceUserObject (objects created by `object { ... }`, modules, scope frames)
 *  */

/* A method entry in a user object. */
typedef struct MethodEntry MethodEntry;

/* Method implementation function type. */
typedef PendingStep *(*MethodFn)(GraceObject *self, Env *env,
                                 GraceObject **args, int nargs, Cont *k,
                                 void *data);

struct MethodEntry {
    const char  *name;   /* e.g. "print(1)", "x(0)", "x:=(1)" */
    MethodFn     fn;
    void        *data;   /* extra state, e.g. captured value */
    MethodEntry *next;
    void       (*trace_data)(void *data);  /* GC: trace GraceObject* refs in data */
    void       (*free_data)(void *data);   /* GC: free ancillary data on sweep */
};

typedef struct {
    GraceObject  base;
    MethodEntry *methods;    /* linked list */
    GraceObject *lex_parent; /* enclosing lexical scope object */
    GraceObject *dialect;    /* dialect object (or NULL) */
    int          is_return_target; /* is this a method activation frame? */
    Cont        *return_k;   /* return continuation (if is_return_target) */
} GraceUserObject;

extern const GraceVTable grace_user_vtable;
GraceObject *grace_user_new(GraceObject *lex_parent);

/* Add a method to a user object. */
void user_add_method(GraceObject *obj, const char *name, MethodFn fn, void *data);

/* Bind a name to a fixed value: adds a zero-arg method returning value.
 * Also adds a setter name:=(1) using a mutable cell. */
void user_bind_def(GraceObject *obj, const char *name, GraceObject *value);

typedef struct { GraceObject **cell; } MutableCell;
void user_bind_var(GraceObject *obj, const char *name, GraceObject **cell);

/* Look up the innermost scope object in the chain that has method `name`.
 * Returns NULL if not found. */
GraceObject *scope_find_receiver(GraceObject *scope, const char *name);

/* Convenience: request a method synchronously.
 * Runs the trampoline internally - useful in converters and prelude helpers. */
GraceObject *grace_request_sync(GraceObject *recv, Env *env, const char *name,
                                 GraceObject **args, int nargs);

/* 
 * Error / exception
 *  */
typedef struct GraceException GraceException;
struct GraceException {
    GraceObject base;
    char       *message;
    char       *tag;        /* e.g. "RuntimeError", "ParseError" */
};
extern const GraceVTable grace_exception_vtable;
GraceObject *grace_exception_new(const char *tag, const char *msg);
/* Create an exception prototype (with raise/refine/match methods). */
GraceObject *grace_exception_proto_new(const char *name);
/* Raise: calls except_k with an exception object, or aborts if NULL. */
void grace_raise(Env *env, const char *tag, const char *fmt, ...);
/* Raise as a pending step - for use inside ongoing CPS chains.
 * Returns a PendingStep* that invokes except_k on the next trampoline tick. */
PendingStep *grace_raise_step(Env *env, const char *tag,
                               const char *fmt, ...);
__attribute__((noreturn)) void grace_fatal(const char *fmt, ...);

/* Synchronous capture continuation - used by grace_request_sync and eval_baked */
typedef struct {
    Cont         base;
    GraceObject *result;
} CaptureCont;
PendingStep *capture_apply(Cont *self, GraceObject *v);

/* 
 * Module registry (for import resolution)
 *  */
void   grace_register_module(const char *name, GraceObject *module);
GraceObject *grace_find_module(const char *name);  /* NULL if not found */

/* 
 * CPS evaluator entry points (implemented in eval.c)
 *  */
/* Evaluate a single AST node in env, pass result to continuation k. */
PendingStep *eval_node(ASTNode *node, Env *env, Cont *k);

/* Evaluate a cons-list of statement nodes; pass final value to k. */
PendingStep *eval_stmts(ASTNode *stmts, Env *env, Cont *k);

/* Evaluate a cons-list of argument nodes to a GraceObject* array,
 * then call done(array, nargs, data). */
typedef PendingStep *(*EvalArgsDone)(GraceObject **args, int nargs,
                                     void *data);
PendingStep *eval_args(ASTNode *args, Env *env,
                       EvalArgsDone done, void *data);

/* Build the method name string from a cons-list of NK_PART nodes.
 * e.g. [p0T("foo",[x],nil), p0T("bar",[y],nil)] -> "foo(1)bar(1)"
 * Result is a freshly-allocated string. */
char *parts_to_name(ASTNode *parts);

/* 
 * Prelude (implemented in prelude.c)
 *  */
/* Create the top-level prelude scope object. */
GraceObject *make_prelude(void);

/* 
 * Converter: GraceObject AST -> ASTNode* (implemented in convert.c)
 *  */
/* Convert the Grace AST object returned by the parser back to an ASTNode*.
 * Runs synchronously using an internal trampoline. */
ASTNode *grace_ast_to_astnode(GraceObject *node, Env *env);

/* 
 * Utility: string helpers
 *  */
/* Heap-allocate a printf-formatted string. */
char *str_fmt(const char *fmt, ...);
/* Concatenate two strings into a new heap-allocated string. */
char *str_cat(const char *a, const char *b);
/* strdup wrapper */
char *str_dup(const char *s);

/* Count elements in a NK_CONS list. */
int list_length(ASTNode *list);

#endif /* GRACE_H */
