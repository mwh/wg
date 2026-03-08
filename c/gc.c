#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "grace.h"
#include "gc.h"

/*
 * Internal state
 */

/* Heap chain: all GC-managed objects linked via gc_next */
static GraceObject *gc_heap_head = NULL;

/* Grey list: objects discovered but not yet traced */
static GraceObject *gc_grey_head = NULL;

/* Sweep cursor for incremental reclamation */
static GraceObject **gc_sweep_prev = NULL;
static GraceObject *gc_sweep_cur = NULL;

/* Root stack */
static GraceObject **gc_root_stack[GC_MAX_ROOTS];
static int gc_root_sp = 0;

/* Trampoline nesting depth */
static int gc_tramp_depth = 0;

/* Allocation counter since last collection */
static size_t gc_alloc_counter = 0;

int gc_phase_start_live;

/* Adaptive threshold: after each collection, set to 2x surviving objects.
 * GC_MIN_THRESHOLD is the floor for the adaptive logic and is intentionally
 * independent of GC_THRESHOLD (the initial trigger value). */
#define GC_MIN_THRESHOLD 2000
static size_t gc_next_threshold = GC_THRESHOLD;

/* Statistics */
static GCStats gc_statistics = {0, 0, 0, 0};

/* Current mark epoch; 0 means "never marked". */
static unsigned int gc_trace_epoch = 1;

static int gc_is_white(GraceObject *obj) {
    return obj && obj->gc_color != GC_STATIC && obj->gc_epoch != gc_trace_epoch;
}

/*
 * Allocation
 */
GraceObject *gc_alloc(size_t size) {
    GraceObject *obj = (GraceObject *)calloc(1, size);
    if (!obj) {
        fprintf(stderr, "gc_alloc: out of memory\n");
        exit(1);
    }
    /* New objects are stamped into the current epoch so they survive any
     * in-progress cycle. If they become unreachable before the next cycle,
     * that next cycle will see them as white automatically. */
    obj->gc_color     = GC_BLACK;
    obj->gc_epoch     = gc_trace_epoch;
    obj->gc_next      = gc_heap_head;
    obj->gc_grey_next = NULL;
    gc_heap_head = obj;

    gc_alloc_counter++;
    gc_statistics.total_allocs++;
    gc_statistics.heap_size++;
    return obj;
}

/*
 * Root stack
 */
void gc_push_root(GraceObject **root) {
    if (gc_root_sp >= GC_MAX_ROOTS) {
        fprintf(stderr, "gc_push_root: root stack overflow\n");
        exit(1);
    }
    gc_root_stack[gc_root_sp++] = root;
}

void gc_pop_root(void) {
    if (gc_root_sp > 0) gc_root_sp--;
}

void gc_pop_roots(int n) {
    gc_root_sp -= n;
    if (gc_root_sp < 0) gc_root_sp = 0;
}

/*
 * Trampoline depth
 */
void gc_trampoline_enter(void) { gc_tramp_depth++; }
void gc_trampoline_exit(void)  { gc_tramp_depth--; }

/*
 * Marking
 */

/* Mark a single object grey (add to grey list) if it is white in this cycle. */
void gc_mark_grey(GraceObject *obj) {
    if (!obj) return;
    if (obj->gc_color == GC_STATIC) return;
    if (obj->gc_epoch == gc_trace_epoch) return;  /* already grey/black this cycle */
    obj->gc_epoch     = gc_trace_epoch;
    obj->gc_color     = GC_GREY;
    obj->gc_grey_next = gc_grey_head;
    gc_grey_head      = obj;
}

/* Trace a continuation: call its gc_trace if set.
 * Uses an epoch check so that shared/repeated continuations are only traced
 * once per GC cycle - this prevents O(chain_length) work when the same
 * continuation is reachable from many live objects (common in deep CPS stacks). */
void gc_trace_cont(Cont *k) {
    if (!k || !k->gc_trace) return;
    if (k->gc_epoch == gc_trace_epoch) return;  /* already traced this cycle */
    k->gc_epoch = gc_trace_epoch;
    k->gc_trace(k);
}

/* Trace an environment: mark receiver/scope and trace continuations */
void gc_trace_env(Env *env) {
    if (!env) return;
    gc_mark_grey(env->receiver);
    gc_mark_grey(env->scope);

    /* Walk the lex_parent chain eagerly so ancestor scopes are queued.
     * Don't stop early at BLACK objects because a BLACK object may be a
     * newly-allocated frame that has never been traced; its ancestors
     * (which have been reset to WHITE) must still be greyed here.
     * Do stop at GREY ancestors because they are already queued and
     * their own user_trace will propagate the grey flag upward. */
    {
        extern const GraceVTable grace_user_vtable;
        GraceObject *c = env->scope;
        if (c && c->vt == &grace_user_vtable)
            c = ((GraceUserObject *)c)->lex_parent;
        while (c && c->vt == &grace_user_vtable) {
            if (c->gc_epoch == gc_trace_epoch && c->gc_color == GC_GREY)
                break;  /* already queued, chain handled */
            gc_mark_grey(c);                     /* grey if WHITE; no-op if BLACK */
            c = ((GraceUserObject *)c)->lex_parent;
        }
        /* grey a non-user terminal node if it is white */
        if (gc_is_white(c)) gc_mark_grey(c);
    }

    gc_trace_cont(env->return_k);
    gc_trace_cont(env->except_k);
}

/* Process the grey list until empty: for each grey object, trace its
 * children (which marks them grey) then colour the object black. */
static void process_grey_list(void) {
    while (gc_grey_head) {
        GraceObject *obj = gc_grey_head;
        gc_grey_head = obj->gc_grey_next;
        obj->gc_grey_next = NULL;

        /* Trace children via vtable */
        if (obj->vt && obj->vt->trace)
            obj->vt->trace(obj);

        obj->gc_color = GC_BLACK;
    }
}

/*
 * Root tracing
 */

typedef void (*GCRootTraceFn)(void);
static GCRootTraceFn gc_extra_root_tracers[8];
static int gc_n_extra_tracers = 0;

void gc_register_root_tracer(GCRootTraceFn fn) {
    if (gc_n_extra_tracers < 8)
        gc_extra_root_tracers[gc_n_extra_tracers++] = fn;
}

static void trace_roots(void) {
    /* 1. Root stack */
    for (int i = 0; i < gc_root_sp; i++) {
        GraceObject *obj = *gc_root_stack[i];
        gc_mark_grey(obj);
    }
    /* 2. Extra root tracers (module registry, current step, etc.) */
    for (int i = 0; i < gc_n_extra_tracers; i++)
        gc_extra_root_tracers[i]();
}

typedef enum { GC_PHASE_IDLE, GC_PHASE_MARKING, GC_PHASE_SWEEPING } GCPhase;
static GCPhase gc_phase = GC_PHASE_IDLE;

static int gc_heap_summary_enabled(void) {
    static int initialised = 0;
    static int enabled = 0;

    if (!initialised) {
        enabled = getenv("GC_HEAP_SUMMARY") != NULL;
        initialised = 1;
    }
    return enabled;
}

static void gc_print_heap_summary(void) {
    size_t numbers = 0;
    size_t strings = 0;
    size_t bools = 0;
    size_t blocks = 0;
    size_t users = 0;
    size_t user_return_targets = 0;
    size_t user_with_lex_parent = 0;
    size_t user_without_methods = 0;
    size_t exceptions = 0;
    size_t other = 0;

    for (GraceObject *obj = gc_heap_head; obj; obj = obj->gc_next) {
        if (obj->vt == &grace_number_vtable) {
            numbers++;
        } else if (obj->vt == &grace_string_vtable) {
            strings++;
        } else if (obj->vt == &grace_bool_vtable) {
            bools++;
        } else if (obj->vt == &grace_block_vtable) {
            blocks++;
        } else if (obj->vt == &grace_user_vtable) {
            GraceUserObject *user = (GraceUserObject *)obj;
            users++;
            if (user->is_return_target)
                user_return_targets++;
            if (user->lex_parent)
                user_with_lex_parent++;
            if (!user->methods)
                user_without_methods++;
        } else if (obj->vt == &grace_exception_vtable) {
            exceptions++;
        } else {
            other++;
        }
    }

    fprintf(stderr,
            "GC %zu heap summary: freed %zu living %zu; user=%zu userLex=%zu userReturn=%zu userNoMethods=%zu number=%zu string=%zu block=%zu bool=%zu exception=%zu other=%zu\n",
            gc_statistics.collections,
            gc_phase_start_live - gc_statistics.heap_size,
            gc_statistics.heap_size,
            users,
            user_with_lex_parent,
            user_return_targets,
            user_without_methods,
            numbers,
            strings,
            blocks,
            bools,
            exceptions,
            other);
}

/*
 * Sweeping
 */

/* Sweep: incrementally walk the heap chain and free objects not marked in the
 * current epoch. Surviving objects keep their current-epoch mark; next cycle
 * will see them as white automatically once gc_trace_epoch advances. */
static void gc_begin_sweep(void) {
    gc_sweep_prev = &gc_heap_head;
    gc_sweep_cur = gc_heap_head;
}

static void gc_finish_cycle(void) {
    size_t surviving = gc_statistics.heap_size;
    gc_next_threshold = (surviving * 2 > GC_MIN_THRESHOLD)
                        ? surviving * 2 : GC_MIN_THRESHOLD;
    gc_phase = GC_PHASE_IDLE;
    gc_alloc_counter = 0;
    gc_statistics.collections++;
    if (gc_heap_summary_enabled())
        gc_print_heap_summary();
    gc_grey_head = NULL;
    gc_sweep_prev = NULL;
    gc_sweep_cur = NULL;
}

static void gc_sweep_slice(int n) {
    for (int i = 0; i < n && gc_sweep_cur; i++) {
        GraceObject *cur = gc_sweep_cur;
        GraceObject *next = cur->gc_next;
        if (cur->gc_color != GC_STATIC && cur->gc_epoch != gc_trace_epoch) {
            *gc_sweep_prev = next;
            if (cur->vt && cur->vt->sweep_free)
                cur->vt->sweep_free(cur);
            free(cur);
            gc_statistics.total_frees++;
            gc_statistics.heap_size--;
        } else {
            gc_sweep_prev = &cur->gc_next;
        }
        gc_sweep_cur = next;
    }
    if (!gc_sweep_cur)
        gc_finish_cycle();
}

/*
 * Collection - incremental tri-colour marking
 *
 * The collector operates as a state machine with three phases:
 *
 *   IDLE    - no cycle in progress; allocate freely until threshold.
 *   MARKING - cycle in progress; each trampoline step processes a small
 *             slice of the grey list until it drains.
 *   SWEEPING - cycle in progress; each trampoline step reclaims a small
 *              slice of unreachable objects.
 *
 * Correctness during incremental marking:
 *   - New objects start BLACK (they were just allocated by the mutator, so
 *     they are definitely live and survive the current cycle).
 *   - The current trampoline step (the live "call stack") is re-greyed at
 *     the start of every mark slice so new root references are captured.
 *   - A write barrier (gc_write_barrier) must be called before storing a
 *     GraceObject* into any mutable field of a live object; it greys the
 *     new value so it cannot be missed by an in-progress mark pass.
 */

/* Number of grey objects to process per trampoline step. */
#define GC_MARK_SLICE 100
#define GC_SWEEP_SLICE 200

/* Begin a new GC cycle: advance epoch and grey roots. Existing objects become
 * implicitly white because their gc_epoch now differs from the new epoch. */
static void gc_start_cycle(void) {
    gc_trace_epoch++;
    if (gc_trace_epoch == 0) gc_trace_epoch = 1;
    gc_phase_start_live = gc_statistics.heap_size;
    gc_grey_head = NULL;
    trace_roots();
    gc_phase = GC_PHASE_MARKING;
    gc_alloc_counter = 0;
}

/* Process up to n grey objects.  Re-greys roots before each slice because
 * the current trampoline step changes every tick; re-tracing ensures new
 * root references added by the mutator between slices are captured.
 * Switches to incremental sweep when the grey list drains completely. */
static void gc_mark_slice(int n) {
    trace_roots();
    for (int i = 0; i < n && gc_grey_head; i++) {
        GraceObject *obj = gc_grey_head;
        gc_grey_head = obj->gc_grey_next;
        obj->gc_grey_next = NULL;
        if (obj->vt && obj->vt->trace)
            obj->vt->trace(obj);
        obj->gc_color = GC_BLACK;
    }
    if (!gc_grey_head) {
        gc_begin_sweep();
        gc_phase = GC_PHASE_SWEEPING;
    }
}

/* Force a complete collection, finishing any in-progress cycle immediately.
 * Intended for explicit calls (e.g. end-of-program cleanup); normal
 * operation uses gc_maybe_collect() instead. */
void gc_collect(void) {
    if (gc_tramp_depth > 1) return;
    if (gc_phase == GC_PHASE_IDLE)
        gc_start_cycle();
    else if (gc_phase == GC_PHASE_MARKING)
        trace_roots();       /* one final root refresh before draining */
    if (gc_phase == GC_PHASE_MARKING) {
        process_grey_list();
        gc_begin_sweep();
    }
    while (gc_sweep_cur)
        gc_sweep_slice(GC_SWEEP_SLICE);
    if (gc_phase != GC_PHASE_IDLE)
        gc_finish_cycle();
}

void gc_maybe_collect(void) {
    if (gc_tramp_depth > 1) return;
    if (gc_phase == GC_PHASE_IDLE) {
        if (gc_alloc_counter >= gc_next_threshold)
            gc_start_cycle();
    } else if (gc_phase == GC_PHASE_MARKING) {
        gc_mark_slice(GC_MARK_SLICE);
    } else {
        gc_sweep_slice(GC_SWEEP_SLICE);
    }
}

/*
 * Write barrier
 */

/* Call before writing a GraceObject* into any mutable field of a live object.
 * During incremental marking, greys the new value so it cannot be missed by
 * the in-progress mark pass. */
void gc_write_barrier(GraceObject *new_val) {
    if (gc_phase == GC_PHASE_MARKING)
        gc_mark_grey(new_val);
}

/*
 * Stats
 */
const GCStats *gc_stats(void) { return &gc_statistics; }

