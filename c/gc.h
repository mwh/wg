/* gc.h - Tri-colour garbage collector for the Grace CPS interpreter */

#ifndef GC_H
#define GC_H

#include <stddef.h>

/* Forward declarations */
typedef struct GraceObject GraceObject;

/*
 * GC colours
 */
#define GC_WHITE  0   /* only used for comments/debugging; whiteness is epoch-relative */
#define GC_GREY   1   /* reachable in current cycle, children not yet traced */
#define GC_BLACK  2   /* reachable in current cycle, fully traced */
#define GC_STATIC 3   /* immortal static singleton, never swept */

/*
 * GC object header fields  (embedded in struct GraceObject)
 *
 *  gc_next       - intrusive linked list of all heap-allocated objects
 *  gc_grey_next  - grey-list chain during marking (NULL if not grey)
 *  gc_color      - one of GC_GREY / GC_BLACK / GC_STATIC in practice
 *  gc_epoch      - mark epoch in which the object was last reached
 */

/*
 * Allocation
 */

/* Allocate a GC-tracked object of `size` bytes.
 * The returned memory is zero-filled (like calloc), linked into the heap,
 * and stamped with the current GC epoch so it survives any in-progress cycle. */
GraceObject *gc_alloc(size_t size);

/*
 * Root management
 */

/* Maximum root-stack depth.  Roots are GraceObject** (pointer to a slot
 * that holds a GraceObject*).  The GC will trace *root for each entry.  */
#define GC_MAX_ROOTS 4096

void gc_push_root(GraceObject **root);
void gc_pop_root(void);
void gc_pop_roots(int n);

/*
 * Trampoline-depth tracking
 */

void gc_trampoline_enter(void);
void gc_trampoline_exit(void);

/*
 * Collection
 */

/* Force a complete collection immediately (finishes any in-progress cycle).
 * Normal operation uses gc_maybe_collect() which runs incrementally. */
void gc_collect(void);

/* Advance incremental marking by one slice, or start a new cycle if the
 * allocation threshold has been reached.  Call from the trampoline loop. */
void gc_maybe_collect(void);

/* Write barrier: call before storing a GraceObject* into any mutable field
 * of a live (GC-managed) object.  Required for incremental marking
 * correctness: greys the new value so it isn't missed by the mark pass. */
void gc_write_barrier(GraceObject *new_val);

/* Initial allocation threshold before the first GC cycle. */
#define GC_THRESHOLD 1000

/*
 * Marking helpers (called from trace functions in objects.c etc.)
 */

/* Mark a single object grey if it is white.  Safe to call with NULL. */
void gc_mark_grey(GraceObject *obj);

/* Trace a Cont chain: calls cont->gc_trace if set. */
void gc_trace_cont(struct Cont *k);

/* Trace an Env: marks receiver and scope, traces return_k and except_k. */
void gc_trace_env(struct Env *env);

/*
 * Extra root tracers
 */

/* Register a callback that will be invoked during the mark phase to
 * mark additional roots (e.g. the module registry). */
typedef void (*GCRootTraceFn)(void);
void gc_register_root_tracer(GCRootTraceFn fn);

/*
 * Statistics (for debugging / logging)
 */
typedef struct {
    size_t total_allocs;
    size_t total_frees;
    size_t collections;
    size_t heap_size;         /* current live objects */
} GCStats;

const GCStats *gc_stats(void);

#endif /* GC_H */
