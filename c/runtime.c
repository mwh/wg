#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <setjmp.h>

struct ObjectMember;
typedef struct ObjectMember ObjectMember_t;
typedef struct GraceObject GraceObject;
typedef struct Context_s Context;
typedef struct ASTNode ASTNode;

struct ObjectMember {
    char *name;
    void *data;
    GraceObject *(*func)(GraceObject *self, int memberIndex, Context *ctx);
    uint8_t flags;
};

typedef struct GraceObject {
    char flags;
    uint32_t refcount;
    uint16_t origin;
    void *data;
    void (*cleanup)(GraceObject*);
    uint8_t n_members;
    ObjectMember_t members[];
} GraceObject;

struct Context_s {
    GraceObject *self;
    GraceObject *scope;
    char *name;
    jmp_buf *return_buf;
};

GraceObject *evaluate(ASTNode *node, Context *context);

typedef struct {
    int n_items;
    ASTNode *items[];
} BodyItem;

typedef struct {
    char nodetype;
    char *name;
    int n_params;
    BodyItem *body;
    ASTNode *params[];
} MethDecNode;

typedef struct ASTNode {
    char nodetype;
} ASTNode;

typedef struct {
    char nodetype;
    double numval;
} NumberNode;

typedef struct {
    char nodetype;
    int defs;
    int vars;
    int methods;
    char **names;
    size_t size;
    ASTNode *body[];
} ObjectNode;

typedef struct {
    char nodetype;
    char *name;
    ASTNode *initialiser;
} DefNode;

typedef struct {
    char nodetype;
    char *name;
    ASTNode *initialiser;
} VarNode;

typedef struct {
    char *name;
    int n_arguments;
    ASTNode *arguments[];
} MethodPart;

typedef struct {
    char nodetype;
    char *name;
    int n_arguments;
    ASTNode *receiver;
    ASTNode *arguments[];
} DotReqNode;

typedef struct {
    char nodetype;
    char *name;
    int n_arguments;
    void *padding; // To match DotReqNode layout
    ASTNode *arguments[];
} LexReqNode;

typedef struct {
    char nodetype;
    char *name;
    ASTNode *type;
} IdentNode;

typedef struct {
    char nodetype;
    char *value;
} StringNode;

typedef struct {
    char nodetype;
    char *start;
    ASTNode *middle;
    ASTNode *after;
} InterpStrNode;

typedef struct {
    char nodetype;
    char *text;
} CommentNode;

typedef struct {
    char nodetype;
    ASTNode *lhs;
    ASTNode *rhs;
} AssignNode;

typedef struct {
    char nodetype;
    ASTNode *expr;
} ReturnNode;

typedef struct {
    char nodetype;
    char *absent; // to align with MethDecNode
    int n_params;
    BodyItem *body;
    ASTNode *params[];
} BlockNode;

typedef struct {
    char nodetype;
    char *path;
    char *as_name;
} ImportNode;

struct ImportRecord {
    char *path;
    GraceObject *module;
    struct ImportRecord *next;
};

#define FLAG_LIVING 1
#define FLAG_FRESH 2
#define FLAG_WRITER 4
#define FLAG_FIELD 8
#define FLAG_FREED 16
#define FLAG_MARKED 32
#define FLAG_IMMORTAL 64

#define OBJECT_NUMBER 1
#define OBJECT_USER 2
#define OBJECT_STRING 3
#define OBJECT_DONE 4
#define OBJECT_PRELUDE 5
#define OBJECT_SCOPE 6
#define OBJECT_BOOLEAN 7
#define OBJECT_BLOCK 8

char *charDollar = "$";
char *charBackslash = "\\";
char *charDQuote = "\"";
char *charLF = "\n";
char *charCR = "\r";
char *charLBrace = "{";
char *charStar = "*";
char *charTilde = "~";
char *charBacktick = "`";
char *charCaret = "^";
char *charAt = "@";
char *charPercent = "%";
char *charAmp = "&";
char *charHash = "#";
char *charExclam = "!";

int GC_DEBUG = 0;
int GC_DISABLE = 0;

int cc_argc;
GraceObject *cc_args[32];


GraceObject *Done;
GraceObject *nonlocal_return_value;

GraceObject *graceNum(double d);
GraceObject *graceString(char *s);
GraceObject *graceBool(int truth);
GraceObject *request(Context *ctx, GraceObject *receiver, char *name, ...);
void dump_object(GraceObject *obj);

struct AllocatedObject {
    GraceObject *obj;
    struct AllocatedObject *next;
};

const char *origin_name(int origin) {
    switch(origin) {
        case OBJECT_USER:    return "user object";
        case OBJECT_NUMBER:  return "number";
        case OBJECT_STRING:  return "string";
        case OBJECT_PRELUDE: return "prelude";
        case OBJECT_DONE:    return "done";
        case OBJECT_SCOPE:   return "scope";
        case OBJECT_BOOLEAN: return "boolean";
        case OBJECT_BLOCK:   return "block";
        default:             return "unknown";
    }
}

struct AllocatedObject *allocated_objects_list = NULL;

struct ImportRecord *imported_modules = NULL;

/// START OBJECTS
void GraceObject__cleanup(GraceObject *obj) {
    if (GC_DEBUG) fprintf(stderr, "cleaned up object of type %i at %p\n", obj->origin, (void *)obj);
}

GraceObject *GraceObject__const_asString(GraceObject *self, int memberIndex, Context *ctx) {
    return graceString(self->members[memberIndex].data);
}

int allocated_objects = 0;
int freed_objects = 0;
GraceObject *prelude;
int ref_validate(GraceObject *obj) {
    if (obj->flags & FLAG_FREED) {
        fprintf(stderr, "Error: use of freed %s object at %p\n", origin_name(obj->origin), (void *)obj);
        exit(1);
    }
    if (obj->refcount < 0) {
        fprintf(stderr, "Error: object at %p has negative refcount %i\n", (void *)obj, obj->refcount);
        exit(1);
    }
    return 1;
}

void ref_inc(GraceObject *obj) {
    ref_validate(obj);
    if (obj->flags & FLAG_FRESH) {
        obj->flags &= ~FLAG_FRESH;
        if (GC_DEBUG) fprintf(stderr, "keeping fresh object %i at %p\n", obj->origin, (void *)obj);
    }
    obj->refcount++;
    if (GC_DEBUG) fprintf(stderr, "ref_inc to %i a %i at %p\n", obj->refcount, obj->origin, (void *)obj);
}

void break_here() {
    return;
}

void ref_discard(GraceObject *obj) {
    ref_validate(obj);
    if (obj->flags & FLAG_IMMORTAL)
        return;
    if (obj->refcount > 0)
        return;
    if (GC_DEBUG) fprintf(stderr, "discarding %s object at %p %i\n", origin_name(obj->origin), (void *)obj, obj->flags);
    if (GC_DEBUG && obj->origin == OBJECT_NUMBER) {
        double *d = obj->data;
        fprintf(stderr, "  value: %f\n", *d);
    }
    obj->cleanup(obj);
    obj->flags &= ~FLAG_LIVING;
    if (GC_DEBUG || GC_DISABLE) {
        obj->flags |= FLAG_FREED;
    } else {
        free(obj);
    }
    freed_objects++;
}

GraceObject *graceBool__false;
void ref_dec(GraceObject *obj) {
    obj->refcount--;
    if (GC_DEBUG) fprintf(stderr, "ref_dec to %i a %i at %p\n", obj->refcount, obj->origin, (void *)obj);
    if (!obj->refcount) {
        ref_discard(obj);
    }
}

GraceObject *alloc_object(int numMembers, int origin) {
    GraceObject *ret = calloc(1, sizeof (struct GraceObject) + numMembers * sizeof(struct ObjectMember));
    if (GC_DEBUG) {
        struct AllocatedObject *entry = malloc(sizeof(struct AllocatedObject));
        entry->obj = ret;
        entry->next = allocated_objects_list;
        allocated_objects_list = entry;
    }
    ret->n_members = numMembers;
    ret->refcount = 0;
    ret->flags = FLAG_LIVING | FLAG_FRESH;
    ret->cleanup = &GraceObject__cleanup;
    ret->origin = origin;
    allocated_objects++;
    return ret;
}
/// END OBJECTS

/// START USER OBJECTS
GraceObject *alloc_userobject(int numMembers);
void add_method(GraceObject *userobject, int index, char *name, MethDecNode *method, Context *context);

GraceObject *userobject_field(GraceObject *self, int memberIndex, Context *ctx) {
    //fprintf(stderr, "userobject_field %i\n", memberIndex);
    if (self->members[memberIndex].flags & FLAG_WRITER) {
        //fprintf(stderr, "it's a writer %s\n", self->members[memberIndex].name);
        struct ObjectMember *other = self->members[memberIndex].data;
        GraceObject *old = other->data;
        ref_inc(cc_args[0]);
        ref_dec(old);
        other->data = cc_args[0];
        cc_args[0]->flags &= ~FLAG_FRESH;
        // fprintf(stderr, "writer setting field %s to %p %i\n", other->name, (void *)cc_args[0], cc_args[0]->refcount);
        // fprintf(stderr, "old value was %p %i\n", (void *)old, old->refcount);
        return Done;
    } else {
        return self->members[memberIndex].data;
    }
}

struct ObjectMember *add_field(GraceObject *userobject, int index, char *name) {
    userobject->members[index].name = name;
    userobject->members[index].func = &userobject_field;
    userobject->members[index].flags = FLAG_FIELD;
    userobject->members[index].data = Done;
    //fprintf(stderr, "created reader %s at %x\n", name, userobject->members[index].func);
    return &userobject->members[index];
}

void add_writer(GraceObject *userobject, int index, char *name, struct ObjectMember *reader) {
    userobject->members[index].name = name;
    userobject->members[index].data = reader;
    userobject->members[index].func = &userobject_field;
    userobject->members[index].flags = FLAG_WRITER;
    //fprintf(stderr, "created writer %s at %i: %x\n", name, index, userobject->members[index].func);
}

void add_var(GraceObject *userobject, int index, char *name1, char *name2) {
    struct ObjectMember *reader = add_field(userobject, index, name1);
    add_writer(userobject, index + 1, name2, reader);
}

GraceObject *userobject_method(GraceObject *self, int memberIndex, Context *ctx) {
    MethDecNode *method = self->members[memberIndex].data;
    BodyItem *body = method->body;
    GraceObject *oldSelf = ctx->self;
    GraceObject *oldScope = ctx->scope;
    jmp_buf *oldReturnBuf;
    oldReturnBuf = ctx->return_buf;
    char *oldName = ctx->name;
    // fprintf(stderr, "invoking method %s with %i params\n", method->name, method->n_params);
    if (self->origin == OBJECT_BLOCK) {
        // Blocks get their context from their data field
        Context *blockContext = self->data;
        ctx->self = blockContext->self;
        ctx->scope = blockContext->scope;
        ctx->return_buf = blockContext->return_buf;
    } else {
        ctx->self = self;
        ctx->scope = self;
    }
    int num_scope_members = method->n_params;
    for (int i = 0; i < body->n_items; i++) {
        if (body->items[i]->nodetype == 'v') {
            num_scope_members += 2;
        } else if (body->items[i]->nodetype == 'd') {
            num_scope_members += 1;
        } else if (body->items[i]->nodetype == 'm') {
            num_scope_members += 1;
        }
    }
    char *names[num_scope_members];
    int name_index = 0;
    for (int i = 0; i < method->n_params; i++) {
        if (method->params[i]->nodetype == 'i') {
            IdentNode *param = (IdentNode*)method->params[i];
            names[name_index++] = param->name;
        } else {
            fprintf(stderr, "Error: non-identifier in method parameters\n");
            exit(1);
        }
    }
    for (int i = 0; i < body->n_items; i++) {
        if (body->items[i]->nodetype == 'v') {
            VarNode *varnode = (VarNode*)body->items[i];
            names[name_index++] = varnode->name;
            int len = strlen(varnode->name);
            char *setter = malloc(len + 6);
            strcpy(setter, varnode->name);
            setter[len] = ':';
            setter[len+1] = '=';
            setter[len+2] = '(';
            setter[len+3] = '1';
            setter[len+4] = ')';
            setter[len+5] = '\0';
            names[name_index++] = setter;
        } else if (body->items[i]->nodetype == 'd') {
            DefNode *defnode = (DefNode*)body->items[i];
            names[name_index++] = defnode->name;
        } else if (body->items[i]->nodetype == 'm') {
            MethDecNode *method = (MethDecNode*)body->items[i];
            names[name_index++] = method->name;
        }
    }

    GraceObject *scope = alloc_userobject(num_scope_members);
    if (GC_DEBUG) fprintf(stderr, "creating scope with %i members for %s: %p\n", num_scope_members, method->name, (void *)scope);
    scope->origin = OBJECT_SCOPE;
    scope->data = ctx->scope;
    ref_inc(ctx->scope);
    ctx->scope = scope;
    ref_inc(scope);
    ctx->name = method->name;

    if (self->origin != OBJECT_BLOCK) {
        jmp_buf env;
        if (setjmp(env)) {
            ctx->self = oldSelf;
            ctx->scope = oldScope;
            ctx->return_buf = oldReturnBuf;
            ctx->name = oldName;
            ref_inc(nonlocal_return_value);
            ref_dec(scope);
            nonlocal_return_value->refcount--; // compensate for the inc above
            return nonlocal_return_value;
        }
        ctx->return_buf = &env;
    }

    int index = 0;
    for (int i = 0; i < method->n_params; i++) {
        if (method->params[i]->nodetype == 'i') {;
            add_field(scope, index, names[index]);
            scope->members[index].data = cc_args[i];
            ref_inc(cc_args[i]);
            index += 1;
        } else {
            fprintf(stderr, "Error: non-identifier in method parameters\n");
            exit(1);
        }
    }
    for (int i = 0; i < method->body->n_items; i++) {
        if (body->items[i]->nodetype == 'v') {
            VarNode *varnode = (VarNode*)body->items[i];
            add_var(scope, index, names[index], names[index + 1]);
            index += 2;
        } else if (body->items[i]->nodetype == 'd') {
            DefNode *defnode = (DefNode*)body->items[i];
            add_field(scope, index, defnode->name);
            index++;
        } else if (body->items[i]->nodetype == 'm') {
            MethDecNode *method = (MethDecNode*)body->items[i];
            add_method(scope, index, method->name, method, ctx);
            index++;
        }
    }
    // fprintf(stderr, "method %s pre-body with scope refcount %i\n", method->name, scope->refcount);

    GraceObject *prev = 0;
    GraceObject *ret = Done;
    for (int i = 0; i < body->n_items; i++) {
        // fprintf(stderr, "about to evaluate body item %i/%i of type %c\n", i, body->n_items, body->items[i]->nodetype);
        ret = evaluate(body->items[i], ctx);
        // fprintf(stderr, "method %s post-body %i with scope refcount %i\n", method->name, i, scope->refcount);
        // fprintf(stderr, "evaluated body item %i/%i of type %c\n", i, body->n_items, body->items[i]->nodetype);
        if (GC_DEBUG) fprintf(stderr, "evaluated body %i, got %i %p\n", i, ret->origin, (void *)ret);
        if (prev != 0)
            ref_discard(prev);
        prev = ret;
        //fprintf(stderr, "after dec %i\n", i);
    }
    if (GC_DEBUG) fprintf(stderr, "end of scope for %s: %p\n", method->name, (void *)scope);
    ctx->self = oldSelf;
    ctx->scope = oldScope;
    ctx->return_buf = oldReturnBuf;
    ctx->name = oldName;
    ref_inc(ret);
    ref_dec(scope);
    ret->refcount--; // compensate for the inc above
    // fprintf(stderr, "method %s exits with scope refcount %i\n", method->name, scope->refcount);
    return ret;
}

void add_method(GraceObject *userobject, int index, char *name, MethDecNode *method, Context *context) {
    userobject->members[index].name = name;
    userobject->members[index].func = &userobject_method;
    userobject->members[index].flags = 0;
    userobject->members[index].data = method;
}

void UserObject__cleanup(GraceObject *obj) {
    if (GC_DEBUG) fprintf(stderr, "cleaned up user object at %p\n", (void *)obj);
    for (int i = 0; i < obj->n_members; i++) {
        if (obj->members[i].flags & FLAG_FIELD) {
            GraceObject *val = obj->members[i].data;
            if (GC_DEBUG) fprintf(stderr, "freeing member %i/%i: %s @ %i\n", i, obj->n_members, obj->members[i].name, val->refcount);
            ref_dec(obj->members[i].data);
        }
    }
    if (obj->data) {
        ref_dec(obj->data);
    }
}

GraceObject *alloc_userobject(int numMembers) {
    GraceObject *ret = alloc_object(numMembers, OBJECT_USER);
    ret->cleanup = &UserObject__cleanup;
    return ret;
}
/// END USER OBJECTS

/// START GRACE NUMBER
GraceObject *(graceNum)(double);

GraceObject *graceNum_asString(GraceObject *self, int memberIndex, Context *ctx) {
    double *my_data = self->data;
    char buf[32];
    if (*my_data == (int)*my_data) {
        sprintf(buf, "%d", (int)*my_data);
        return graceString(buf);
    }
    sprintf(buf, "%f", *my_data);
    return graceString(buf);
}

GraceObject *graceNum_plus(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceNum(*my_data + *its_data);
}

GraceObject *graceNum_sub(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceNum(*my_data - *its_data);
}

GraceObject *graceNum_div(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceNum(*my_data / *its_data);
}

GraceObject *graceNum_mul(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceNum(*my_data * *its_data);
}

GraceObject *graceNum_lt(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceBool(*my_data < *its_data);
}

GraceObject *graceNum_lte(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceBool(*my_data <= *its_data);
}

GraceObject *graceNum_gt(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceBool(*my_data > *its_data);
}

GraceObject *graceNum_gte(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceBool(*my_data >= *its_data);
}

GraceObject *graceNum_eq(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceBool(*my_data == *its_data);
}

GraceObject *graceNum_neq(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    double *my_data = self->data;
    double *its_data = other->data;
    return graceBool(*my_data != *its_data);
}

void GraceNum__cleanup(GraceObject *obj) {
    if (GC_DEBUG) fprintf(stderr, "cleaned up number %f at %p\n", *((double *)obj->data), (void *)obj);
    if (!GC_DEBUG && !GC_DISABLE) free(obj->data);
}

GraceObject *graceNum(double d) {
    GraceObject *ret = alloc_object(11, OBJECT_NUMBER);
    ret->members[0].name = "asString";
    ret->members[0].func = &graceNum_asString;
    ret->members[1].name = "+(1)";
    ret->members[1].func = &graceNum_plus;
    ret->members[2].name = "-(1)";
    ret->members[2].func = &graceNum_sub;
    ret->members[3].name = "*(1)";
    ret->members[3].func = &graceNum_mul;
    ret->members[4].name = "/(1)";
    ret->members[4].func = &graceNum_div;
    ret->members[5].name = "<(1)";
    ret->members[5].func = &graceNum_lt;
    ret->members[6].name = "<=(1)";
    ret->members[6].func = &graceNum_lte;
    ret->members[7].name = ">(1)";
    ret->members[7].func = &graceNum_gt;
    ret->members[8].name = ">=(1)";
    ret->members[8].func = &graceNum_gte;
    ret->members[9].name = "==(1)";
    ret->members[9].func = &graceNum_eq;
    ret->members[10].name = "!=(1)";
    ret->members[10].func = &graceNum_neq;
    ret->cleanup = &GraceNum__cleanup;
    ret->data = malloc(sizeof(double));
    *((double *)ret->data) = d;
    if (GC_DEBUG) fprintf(stderr, "created number %f at %p\n", d, (void *)ret);
    return ret;
}
/// END GRACE NUMBER

/// START GRACE STRING

struct GraceStringData {
    int length;
    char *str;
};

void GraceString__cleanup(GraceObject *obj) {
    struct GraceStringData *data = obj->data;
    if (GC_DEBUG) fprintf(stderr, "cleaned up string %s at %p\n", data->str, (void *)obj);
    if (!GC_DEBUG && !GC_DISABLE) {
        free(data->str);
        free(data);
    }
}

GraceObject *graceString_replace_with(GraceObject *self, int memberIndex, Context *ctx) {
    struct GraceStringData *data = self->data;
    char *src = data->str;
    struct GraceStringData *needleArg = cc_args[0]->data;
    struct GraceStringData *replacementArg = cc_args[1]->data;
    char *needle = needleArg->str;
    char *replacement = replacementArg->str;
    char *buf;
    int bufsize;
    if (needleArg->length >= replacementArg->length) {
        bufsize = data->length;
        buf = malloc(bufsize + 1);
        buf[0] = 0;
    } else {
        bufsize = data->length + (replacementArg->length - needleArg->length) * 10;
        buf = malloc(bufsize + 1);
        buf[0] = 0;
    }
    char *p = src;
    char *q;
    while ((q = strstr(p, needle)) != NULL) {
        int n = q - p;
        if (strlen(buf) + n + replacementArg->length > bufsize) {
            bufsize *= 2;
            buf = realloc(buf, bufsize + 1);
        }
        strncat(buf, p, n);
        strcat(buf, replacement);
        p = q + needleArg->length;
    }
    strcpy(buf + (p - src), p);
    GraceObject *ret = graceString(buf);
    free(buf);
    return ret;
}

GraceObject *graceString_substringFrom_to(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *fromObj = cc_args[0];
    GraceObject *toObj = cc_args[1];
    if (fromObj->origin != OBJECT_NUMBER || toObj->origin != OBJECT_NUMBER) {
        // TODO: error handling
        return 0;
    }
    struct GraceStringData *data = self->data;
    char *str = data->str;
    int from = *((double *)fromObj->data) - 1;
    int to = *((double *)toObj->data);
    if (to > data->length)
        to = data->length;
    if (from < 0)
        from = 0;
    if (from > to) {
        char *buf = malloc(1);
        buf[0] = 0;
        return graceString(buf);
    }
    int len = to - from;
    char *buf = malloc(len + 1);
    strncpy(buf, str + from, len);
    buf[len] = 0;
    return graceString(buf);
}

GraceObject *graceString_asString(GraceObject *self, int memberIndex, Context *ctx) {
    return self;
}

GraceObject *graceString_concat(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    int mustClean = 0;
    if (other->origin != self->origin) {
        other = request(ctx, other, "asString", 0, ctx);
        mustClean = 1;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    int new_length = my_data->length + its_data->length;
    char buf[new_length + 1];
    buf[0] = 0;
    strcpy(buf, my_data->str);
    strcat(buf, its_data->str);
    if (mustClean)
        ref_discard(other);
    return graceString(buf);
}

GraceObject *graceString_size(GraceObject *self, int memberIndex, Context *ctx) {
    struct GraceStringData *my_data = self->data;
    return graceNum(my_data->length);
}

// TODO: utf-8
GraceObject *graceString_at(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *indexObj = cc_args[0];
    if (indexObj->origin != OBJECT_NUMBER) {
        // TODO: errors
        fprintf(stderr, "Error: string index is not a number\n");
        exit(1);
    }
    struct GraceStringData *my_data = self->data;
    int index = *((double *)indexObj->data) - 1;
    if (index < 0 || index >= my_data->length) {
        fprintf(stderr, "Error: string index out of bounds: %i <= %i < %i\n", 0, index, my_data->length);
        exit(1);
    }
    char buf[2];
    buf[0] = my_data->str[index];
    buf[1] = 0;
    return graceString(buf);
}

GraceObject *graceString_eq(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    return graceBool(strcmp(my_data->str, its_data->str) == 0);
}

GraceObject *graceString_neq(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    return graceBool(strcmp(my_data->str, its_data->str) != 0);
}

GraceObject *graceString_lt(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    return graceBool(strcmp(my_data->str, its_data->str) < 0);
}

GraceObject *graceString_lte(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    return graceBool(strcmp(my_data->str, its_data->str) <= 0);
}

GraceObject *graceString_gt(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    return graceBool(strcmp(my_data->str, its_data->str) > 0);
}

GraceObject *graceString_gte(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    struct GraceStringData *my_data = self->data;
    struct GraceStringData *its_data = other->data;
    return graceBool(strcmp(my_data->str, its_data->str) >= 0);
}

GraceObject *graceString_firstCodepoint(GraceObject *self, int memberIndex, Context *ctx) {
    struct GraceStringData *my_data = self->data;
    if (my_data->length == 0) {
        return graceNum(0);
    }
    return graceNum((unsigned char)my_data->str[0]);
}

GraceObject *graceString(char *str) {
    GraceObject *ret = alloc_object(13, OBJECT_STRING);
    ret->members[0].name = "asString";
    ret->members[0].func = &graceString_asString;
    ret->members[1].name = "++(1)";
    ret->members[1].func = &graceString_concat;
    ret->members[2].name = "size";
    ret->members[2].func = &graceString_size;
    ret->members[3].name = "at(1)";
    ret->members[3].func = &graceString_at;
    ret->members[4].name = "==(1)";
    ret->members[4].func = &graceString_eq;
    ret->members[5].name = "!=(1)";
    ret->members[5].func = &graceString_neq;
    ret->members[6].name = "<(1)";
    ret->members[6].func = &graceString_lt;
    ret->members[7].name = "<=(1)";
    ret->members[7].func = &graceString_lte;
    ret->members[8].name = ">(1)";
    ret->members[8].func = &graceString_gt;
    ret->members[9].name = ">=(1)";
    ret->members[9].func = &graceString_gte;
    ret->members[10].name = "firstCodepoint";
    ret->members[10].func = &graceString_firstCodepoint;
    ret->members[11].name = "replace(1)with(1)";
    ret->members[11].func = &graceString_replace_with;
    ret->members[12].name = "substringFrom(1)to(1)";
    ret->members[12].func = &graceString_substringFrom_to;
    ret->cleanup = &GraceString__cleanup;
    ret->data = malloc(sizeof(struct GraceStringData));
    struct GraceStringData *data = ret->data;
    data->length = strlen(str);
    data->str = malloc(data->length + 1);
    strcpy(data->str, str);
    if (GC_DEBUG) fprintf(stderr, "created string %s at %p\n", data->str, (void *)ret);
    return ret;
}
/// END GRACE STRING

/// START GRACE BOOLEAN
GraceObject *(graceBool)(int);

GraceObject *graceBool__true;
GraceObject *graceBool__false;

GraceObject *graceBool_asString(GraceObject *self, int memberIndex, Context *ctx) {
    if (self->data) {
        return graceString("true");
    } else {
        return graceString("false");
    }
}

GraceObject *graceBool_not(GraceObject *self, int memberIndex, Context *ctx) {
    return graceBool(!self->data);
}

GraceObject *graceBool_and(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        fprintf(stderr, "Error: AND with non-boolean\n");
        exit(1);
    }
    return graceBool(self->data && other->data);
}

GraceObject *graceBool_or(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        // TODO: errors
        return 0;
    }
    return graceBool(self->data || other->data);
}

GraceObject *graceBool_eq(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        return graceBool__false;
    }
    return graceBool(self->data == other->data);
}

GraceObject *graceBool_neq(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *other = cc_args[0];
    if (other->origin != self->origin) {
        return graceBool__true;
    }
    return graceBool(self->data != other->data);
}

void GraceBool__cleanup(GraceObject *obj) {

}

GraceObject *graceBool(int truth) {
    if (truth && graceBool__true) {
        return graceBool__true;
    } else if (!truth && graceBool__false) {
        return graceBool__false;
    }
    GraceObject *ret = alloc_object(6, OBJECT_BOOLEAN);
    ret->members[0].name = "asString";
    ret->members[0].func = &graceBool_asString;
    ret->members[1].name = "&&(1)";
    ret->members[1].func = &graceBool_and;
    ret->members[2].name = "prefix!";
    ret->members[2].func = &graceBool_not;
    ret->members[3].name = "||(1)";
    ret->members[3].func = &graceBool_or;
    ret->members[4].name = "==(1)";
    ret->members[4].func = &graceBool_eq;
    ret->members[5].name = "!=(1)";
    ret->members[5].func = &graceBool_neq;
    ret->cleanup = &GraceBool__cleanup;
    ret->data = truth ? (void *)1 : (void *)0;
    ret->flags |= FLAG_IMMORTAL;
    if (GC_DEBUG) fprintf(stderr, "created boolean %d at %p\n", truth, (void *)ret);
    if (truth) {
        graceBool__true = ret;
    } else {
        graceBool__false = ret;
    }
    return ret;
}
/// END GRACE BOOL

/// START GRACE BLOCK

GraceObject *block_apply(GraceObject *self, int memberIndex, Context *ctx) {
    return Done;
}

void block__cleanup(GraceObject *obj) {
    if (GC_DEBUG) fprintf(stderr, "cleaned up block at %p\n", (void *)obj);
    if (obj->data) {
        Context *context = obj->data;
        ref_dec(context->self);
        ref_dec(context->scope);
        if (!GC_DEBUG && !GC_DISABLE) free(context);
    }
}

GraceObject *graceBlock(BlockNode *bn, Context *context) {
    GraceObject *block = alloc_userobject(1);
    block->origin = OBJECT_BLOCK;
    if (bn->n_params == 0) {
        block->members[0].name = "apply";
    } else {
        char buf[20];
        sprintf(buf, "apply(%i)", bn->n_params);
        block->members[0].name = strdup(buf);
    }
    block->members[0].func = &userobject_method;
    block->members[0].data = bn;
    //block->data = context;
    ref_inc(context->self);
    ref_inc(context->scope);
    Context *ctxCopy = malloc(sizeof(Context));
    memcpy(ctxCopy, context, sizeof(Context));
    block->data = ctxCopy;
    block->cleanup = &block__cleanup;
    return block;
}

/// END GRACE BLOCK


/// START REQUESTS
GraceObject *do_request(Context *ctx, GraceObject *receiver, char *name) {
    int i;
    // fprintf(stderr, "request of %s\n", name);
    GraceObject *ret = 0;
    GraceObject *args[cc_argc];
    int argc = cc_argc;
    if (GC_DEBUG) fprintf(stderr, "do_request %s on %p with %i args\n", name, (void *)receiver, argc);
    ref_validate(ctx->self);
    // fprintf(stderr, "pre-scope val %s\n", name);
    ref_validate(ctx->scope);
    // fprintf(stderr, "post-scope val %s\n", name);
    ref_validate(receiver);
    ref_inc(receiver);
    for (i = 0; i < cc_argc; i++) {
        // fprintf(stderr, "ref_inc arg %i: %p %i %s\n", i, (void *)cc_args[i], cc_args[i]->refcount, name);
        args[i] = cc_args[i];
        ref_inc(args[i]);
        if (args[i]->n_members == 23) {
            //fprintf(stderr, "lexer as arg to %s with rc %i\n", name, args[i]->refcount);
        }
    }
    int found = 0;
    // fprintf(stderr, "requesting %s on %s object\n", name, origin_name(receiver->origin));
    for (i = 0; i < receiver->n_members; i++) {
        // fprintf(stderr, "  checking member %i: %s\n", i, receiver->members[i].name);
        if (strcmp(name, receiver->members[i].name) == 0) {
            // fprintf(stderr, "found member at index %i: %x\n", i, receiver->members[i].func);
            ret = receiver->members[i].func(receiver, i, ctx);
            found = 1;
            break;
        } else {
            //fprintf(stderr, "no match at %i: %s != %s\n", i, name, receiver->members[i].name);
        }
    }
    for (i = 0; i < argc; i++) {
        // fprintf(stderr, "ref_dec arg %i: %p %i %s\n", i, (void *)args[i], args[i]->refcount, name);
        // fprintf(stderr, "ref_dec done\n");
        if (args[i] != ret)
            ref_dec(args[i]);
    }
    if (!found) {
        fprintf(stderr, "Error: method %s not found on %s object\n", name, origin_name(receiver->origin));
        exit(1);
    }
    if (ret != receiver) ref_dec(receiver);
    if (GC_DEBUG) fprintf(stderr, "do_request returning %p (rc %i) from %s\n", (void *)ret, ret->refcount, name);
    // TODO: errors
    // fprintf(stderr, "pre-return validate %s\n", name);
    ref_validate(ret);
    // fprintf(stderr, "post-return validate %s\n", name);
    return ret;
}

GraceObject *request(Context *ctx, GraceObject *receiver, char *name, ...) {
    va_list args;
    int i;
    va_start(args, name);
    //fprintf(stderr, "requesting %s\n", name);
    for (i = 0; ; i++) {
        GraceObject *next = va_arg(args, GraceObject*);
        if (next == 0)
            break;
        cc_args[i] = next;
    }
    cc_argc = i;
    va_end(args);

    //fprintf(stderr, "about to do request %s on %x\n", name, receiver);
    return do_request(ctx, receiver, name);
}
/// END REQUESTS



ASTNode *program;

ASTNode *numLit(double d) {
    NumberNode *ret = malloc(sizeof(NumberNode));
    ret->nodetype = 'n';
    ret->numval = d;
    return (ASTNode*)ret;
}

ASTNode *strLit(char *s) {
    StringNode *ret = malloc(sizeof(StringNode));
    ret->nodetype = 's';
    ret->value = s;
    return (ASTNode*)ret;
}

ASTNode *interpStr(char *start, ASTNode *middle, ASTNode *after) {
    InterpStrNode *ret = malloc(sizeof(InterpStrNode));
    ret->nodetype = 'S';
    ret->start = start;
    ret->middle = middle;
    ret->after = after;
    return (ASTNode*)ret;
}

typedef struct ConsCell ConsCell;
int cons_count = 0;
int cons_freed = 0;

struct ConsCell {
    void *head;
    struct ConsCell *tail;
};

void free_cons(ConsCell *cell) {
    int this_time = 0;
    while (cell) {
        cons_freed++;
        this_time++;
        ConsCell *next = cell->tail;
        if (GC_DEBUG) fprintf(stderr, "freeing cell %p %i/%i (%i)\t\t%p\n", cell, cons_freed, cons_count, this_time, next);
        free(cell);
        cell = next;
    }
}

void free_cons_both(ConsCell *cell) {
    int this_time = 0;
    while (cell) {
        cons_freed++;
        this_time++;
        ConsCell *next = cell->tail;
        if (GC_DEBUG) fprintf(stderr, "freeing both %p %i/%i (%i)\t\t%p\n", cell, cons_freed, cons_count, this_time, next);
        free(cell->head);
        free(cell);
        cell = next;
    }
}

MethodPart *part(char *name, ConsCell *args) {
    int size = 0;
    ConsCell *tmp = args;
    while (tmp) {
        size++;
        tmp = tmp->tail;
    }
    MethodPart *ret = malloc(sizeof(MethodPart) + sizeof(ASTNode*) * size);
    // fprintf(stderr, "creating method part %s with %i args at %p\n", name, size, (void *)ret);
    ret->name = name;
    ret->n_arguments = size;
    tmp = args;
    for (int i = 0; i < size; i++) {
        ret->arguments[i] = tmp->head;
        tmp = tmp->tail;
    }
    free_cons(args);
    return ret;
}

ASTNode *parts_handler(ASTNode *receiver, ConsCell *parts) {
    int size = 0;
    ConsCell *tmp = parts;
    int name_len = 0;
    char buf[10];
    int num_args = 0;
    while (tmp) {
        size++;
        MethodPart *p = tmp->head;
        sprintf(buf, "%i", p->n_arguments);
        name_len += strlen(p->name) + strlen(buf) + 2;
        num_args += p->n_arguments;
        tmp = tmp->tail;
    }
    DotReqNode *ret = malloc(sizeof(DotReqNode) + sizeof(ASTNode*) * num_args);
    ret->nodetype = 'r';
    ret->n_arguments = num_args;
    char *name = malloc(name_len + 1);
    char *upto = name;
    int arg_i = 0;
    name[0] = 0;
    ret->receiver = receiver;
    if (size == 1 && num_args == 0) {
        // Leave off "(0)"
        MethodPart *p = parts->head;
        strcpy(name, p->name);
    } else {
        tmp = parts;
        for (int i = 0; i < size; i++) {
            MethodPart *p = tmp->head;
            upto += sprintf(upto, "%s(%i)", p->name, p->n_arguments);
            for (int j = 0; j < p->n_arguments; j++) {
                ret->arguments[arg_i] = p->arguments[j];
                arg_i++;
            }
            tmp = tmp->tail;
        }
    }
    ret->name = name;
    return (ASTNode *)ret;
}

ASTNode *dotReq(ASTNode *receiver, ConsCell *parts) {
    ASTNode *ret = parts_handler(receiver, parts);
    free_cons_both(parts);
    return ret;
}

ASTNode *lexReq(ConsCell *parts) {
    ASTNode *ret = parts_handler(0, parts);
    ret->nodetype = 'l';
    free_cons_both(parts);
    return ret;
}

ASTNode *objCons(ConsCell *body, ConsCell *annotations) {
    int i;
    int size = 0;
    ConsCell *tmp = body;
    int defs = 0;
    int vars = 0;
    int methods = 0;
    while (tmp) {
        size++;
        ASTNode *node = tmp->head;
        if (node->nodetype == 'v')
            vars++;
        if (node->nodetype == 'd' || node->nodetype == 'I')
            defs++;
        if (node->nodetype == 'm')
            methods++;
        tmp = tmp->tail;
    }
    char **names = malloc((defs + methods + vars * 2) * sizeof(char *));
    tmp = body;
    i = 0;
    while (tmp) {
        ASTNode *node = tmp->head;
        if (node->nodetype == 'v') {
            char *name = ((VarNode*)node)->name;
            names[i++] = name;
            int len = strlen(name);
            char *setter = malloc(len + 6);
            strcpy(setter, name);
            setter[len] = ':';
            setter[len+1] = '=';
            setter[len+2] = '(';
            setter[len+3] = '1';
            setter[len+4] = ')';
            setter[len+5] = '\0';
            names[i++] = setter;
        }
        if (node->nodetype == 'd')
            names[i++] = ((DefNode*)node)->name;
        if (node->nodetype == 'm')
            names[i++] = ((MethDecNode*)node)->name;
        if (node->nodetype == 'I')
            names[i++] = ((ImportNode*)node)->as_name;
        tmp = tmp->tail;
    }

    ObjectNode *ret = malloc(sizeof(ObjectNode) + size * sizeof(ASTNode *));
    ret->nodetype = 'o';
    ret->vars = vars;
    ret->defs = defs;
    ret->methods = methods;
    ret->names = names;
    ret->size = size;
    tmp = body;
    for (i = 0; i < size; i++) {
        ret->body[i] = tmp->head;
        tmp = tmp->tail;
    }
    free_cons(body);
    free_cons(annotations);
    return (ASTNode *)ret;
}

ASTNode *varDec(char *name, ConsCell *type, ConsCell *anns, ConsCell *init) {
    VarNode *ret = malloc(sizeof(VarNode));
    ret->nodetype = 'v';
    ret->name = name;
    if (init)
        ret->initialiser = init->head;
    else
        ret->initialiser = 0;
    free_cons(type);
    free_cons(anns);
    free_cons(init);
    return (ASTNode*)ret;
}

ASTNode *defDec(char *name, ConsCell *type, ConsCell *anns, ASTNode *init) {
    DefNode *ret = malloc(sizeof(DefNode));
    ret->nodetype = 'd';
    ret->name = name;
    ret->initialiser = init;
    free_cons(type);
    free_cons(anns);
    return (ASTNode*)ret;
}

ASTNode *assn(ASTNode *lhs, ASTNode *rhs) {
    AssignNode *ret = malloc(sizeof(AssignNode));
    ret->nodetype = '=';
    ret->lhs = lhs;
    ret->rhs = rhs;
    return (ASTNode*)ret;
}

ASTNode *methDec(ConsCell *parts, ConsCell *type, ConsCell *anns, ConsCell *body) {
    ASTNode *ret = parts_handler(0, parts);
    ret->nodetype = 'm';
    MethDecNode *m = (MethDecNode*)ret;
    ConsCell *tmp = parts;
    int n_params = 0;
    while (tmp) {
        MethodPart *p = tmp->head;
        n_params += p->n_arguments;
        tmp = tmp->tail;
    }
    m->n_params = n_params;
    tmp = body;
    int n_items = 0;
    while (tmp) {
        n_items++;
        tmp = tmp->tail;
    }
    m->body = malloc(sizeof(BodyItem) + n_items * sizeof(ASTNode *));
    m->body->n_items = n_items;
    tmp = body;
    for (int i = 0; i < n_items; i++) {
        m->body->items[i] = tmp->head;
        tmp = tmp->tail;
    }
    free_cons_both(parts);
    free_cons(type);
    free_cons(anns);
    free_cons(body);
    return ret;
}

ASTNode *block(ConsCell *params, ConsCell *body) {
    int n_params = 0;
    ConsCell *tmp = params;
    while (tmp) {
        n_params++;
        tmp = tmp->tail;
    }
    BlockNode *ret = malloc(sizeof(BlockNode) + n_params * sizeof(ASTNode *));
    ret->nodetype = 'b';
    ret->absent = "<block>";
    ret->n_params = n_params;
    tmp = params;
    for (int i = 0; i < n_params; i++) {
        ret->params[i] = tmp->head;
        tmp = tmp->tail;
    }
    int n_items = 0;
    tmp = body;
    while (tmp) {
        n_items++;
        tmp = tmp->tail;
    }
    ret->body = malloc(sizeof(BodyItem) + n_items * sizeof(ASTNode *));
    ret->body->n_items = n_items;
    tmp = body;
    for (int i = 0; i < n_items; i++) {
        ret->body->items[i] = tmp->head;
        tmp = tmp->tail;
    }
    free_cons(params);
    free_cons(body);
    return (ASTNode*)ret;
}

ASTNode *identifierDeclaration(char *name, ConsCell *type) {
    IdentNode *ret = malloc(sizeof(IdentNode));
    ret->nodetype = 'i';
    ret->name = name;
    ret->type = 0;
    free_cons(type);
    return (ASTNode*)ret;
}

ASTNode *comment(char *text) {
    CommentNode *ret = malloc(sizeof(CommentNode));
    ret->nodetype = '#';
    ret->text = text;
    return (ASTNode*)ret;
}

ASTNode *returnStmt(ASTNode *expr) {
    ReturnNode *ret = malloc(sizeof(ReturnNode));
    ret->nodetype = 'R';
    ret->expr = expr;
    return (ASTNode*)ret;
}

ASTNode *importStmt(char *path, ASTNode *as_name) {
    ImportNode *ret = malloc(sizeof(ImportNode));
    ret->nodetype = 'I';
    ret->path = path;
    IdentNode *as_name_node = (IdentNode*)as_name;
    if (as_name_node && as_name_node->nodetype == 'i')
        ret->as_name = as_name_node->name;
    else {
        fprintf(stderr, "Error: import ... as requires an identifier\n");
        exit(1);
    }
    return (ASTNode*)ret;
}


ConsCell *cons(void *value, ConsCell *next) {
    cons_count++;
    ConsCell *ret = malloc(sizeof(ConsCell));
    ret->head = value;
    ret->tail = next;
    return ret;
}

#define nil 0
#define ozne(x) cons(x, 0)

ConsCell *one(void *x) { return cons(x, 0); }

char *safeStr(char *pre, char *mid, char *post) {
    int len = strlen(pre) + strlen(mid) + strlen(post);
    char *ret = malloc(len + 1);
    strcpy(ret, pre);
    strcat(ret, mid);
    strcat(ret, post);
    return ret;
}


struct ObjectMember *findField(Context *ctx, char *name) {
    GraceObject *scope = ctx->scope;

    while (scope) {
        if (scope->origin & (OBJECT_USER | OBJECT_PRELUDE | OBJECT_SCOPE)) {
            int i;
            for (i = 0; i < scope->n_members; i++) {
                if (strcmp(scope->members[i].name, name) == 0) {
                    return &scope->members[i];
                }
            }
        }
        scope = 0;
    }
    return 0;
}

GraceObject *findReceiver(Context *ctx, char *name) {
    GraceObject *scope = ctx->scope;

    while (scope) {
        // fprintf(stderr, "looking for receiver of %s in scope %p\n", name, scope);
        if (scope->origin & (OBJECT_USER | OBJECT_PRELUDE | OBJECT_SCOPE)) {
            int i;
            for (i = 0; i < scope->n_members; i++) {
                // fprintf(stderr, "  checking member %s\n", scope->members[i].name);
                if (strcmp(scope->members[i].name, name) == 0) {
                    return scope;
                }
            }
            scope = scope->data;
        } else {
            // ran out of nested scopes
            return 0;
        }
    }
    return 0;
}

void debugFindReceiver(Context *ctx, char *name) {
    GraceObject *scope = ctx->scope;

    fprintf(stderr, "Searching for \"%s\"\n", name);
    int scopePos = 1;
    while (scope) {
        if (scope->origin & (OBJECT_USER | OBJECT_PRELUDE | OBJECT_SCOPE)) {
            fprintf(stderr, "  %i: %s %p (n_members %i)\n", scopePos++, origin_name(scope->origin), (void *)scope, scope->n_members);
            int i;
            for (i = 0; i < scope->n_members; i++) {
                fprintf(stderr, "    - %s\n", scope->members[i].name);
                if (strcmp(scope->members[i].name, name) == 0) {
                    fprintf(stderr, "  Found in scope %p\n", (void *)scope);
                    return;
                }
            }
            scope = scope->data;
        } else {
            fprintf(stderr, "  Reached non-scope object %p %i %s\n", (void *)scope, scope->origin, origin_name(scope->origin));
            scope = 0;
        }
    }
    fprintf(stderr, "  Not found.\n");
    return;
}

GraceObject *evaluateNumber(NumberNode *num, Context *context) {
    return graceNum(num->numval);
}

GraceObject *evaluateStrLit(StringNode *str, Context *context) {
    return graceString(str->value);
}

GraceObject *evaluateObject(ObjectNode *obj, Context *context) {
    //fprintf(stderr, "evaluating object, size %i\n", obj->size);
    GraceObject *ret = alloc_userobject(obj->methods + obj->defs + obj->vars * 2);
    ref_inc(ret);
    ret->flags &= ~FLAG_FRESH;
    if (GC_DEBUG) fprintf(stderr, "created user object at %p\n", (void *)ret);
    ret->data = context->scope;
    ref_inc(ret->data);
    int i;
    int index = 0;
    for (i = 0; i < obj->size; i++) {
        if (obj->body[i]->nodetype == 'v') {
            //fprintf(stderr, "creating var with name at %x\n", obj->names[index]);
            VarNode *varnode = (VarNode*)obj->body[i];
            add_writer(ret, index + 1, obj->names[index + 1], add_field(ret, index, obj->names[index]));
            index += 2;
        } else if (obj->body[i]->nodetype == 'd') {
            //fprintf(stderr, "creating def with name at %x\n", obj->names[index]);
            DefNode *defnode = (DefNode*)obj->body[i];
            add_field(ret, index, obj->names[index]);
            index += 1;
        } else if (obj->body[i]->nodetype == 'm') {
            //fprintf(stderr, "creating method with name at %x\n", obj->names[index]);
            MethDecNode *method = (MethDecNode*)obj->body[i];
            add_method(ret, index, method->name, method, context);
            index += 1;
        } else if (obj->body[i]->nodetype == 'I') {
            ImportNode *importnode = (ImportNode*)obj->body[i];
            add_field(ret, index, obj->names[index]);
            index += 1;
        } else {
            //fprintf(stderr, "unhandled nodetype %c\n", obj->body[i]->nodetype);
        }
    }
    GraceObject *oldSelf = context->self;
    GraceObject *oldScope = context->scope;
    context->self = ret;
    context->scope = ret;
    for (i = 0; i < obj->size; i++) {

        if (GC_DEBUG) fprintf(stderr, "evaluating body %i, a %c\n", i, obj->body[i]->nodetype);
        GraceObject *result = evaluate(obj->body[i], context);
        if (GC_DEBUG) fprintf(stderr, "evaluated body %i, got %i %p\n", i, result->origin, (void *)result);
        ref_discard(result);
    }
    context->self = oldSelf;
    context->scope = oldScope;
    ret->flags |= FLAG_FRESH;
    ret->refcount--; // soft dec so isn't freed yet
    return ret;
}

GraceObject *evaluateVar(VarNode *vn, Context *context) {
    // fprintf(stderr, "evaluating var %s\n", vn->name);
    struct ObjectMember *field = findField(context, vn->name);
    if (!field) {
        fprintf(stderr, "Error: could not find field for var %s\n", vn->name);
        exit(1);
    }
    if (!vn->initialiser) {
        return Done;
    }
    GraceObject *value = evaluate(vn->initialiser, context);
    // fprintf(stderr, "evaluated var %s initialiser to %i %p\n", vn->name, value->origin, (void *)value);
    ref_inc(value);
    field->data = value;
    // fprintf(stderr, "initialized var %s to %i %p\n", vn->name, value->origin, (void *)value);
    return Done;
}

GraceObject *evaluateDef(DefNode *dn, Context *context) {
    struct ObjectMember *field = findField(context, dn->name);
    GraceObject *value = evaluate(dn->initialiser, context);
    ref_inc(value);
    // if (strcmp(dn->name, "pending") == 0) {
    //     fprintf(stderr, "Defining pending to %i %p rc %i\n", value->origin, (void *)value, value->refcount);
    // }
    field->data = value;
    return Done;
}

GraceObject *evaluateDotRequest(DotReqNode *dr, Context *context) {
    //fprintf(stderr, "evaluating dotreq\n");
    GraceObject *receiver = evaluate(dr->receiver, context);
    if (!receiver) {
        fprintf(stderr, "Error: receiver %c evaluated to null in %s\n", dr->receiver->nodetype, context->name);
        exit(1);
    }
    ref_inc(receiver);
    //fprintf(stderr, "got receiver %x\n", receiver);
    //fprintf(stderr, "arguments to find: %i\n", dr->n_arguments);
    GraceObject *args[dr->n_arguments];
    for (int i = 0; i < dr->n_arguments; i++) {
        //fprintf(stderr, "evaluating argument %i\n", i);
        args[i] = evaluate(dr->arguments[i], context);
        // ref_inc(args[i]);
    }
    for (int i = 0; i < dr->n_arguments; i++) {
        cc_args[i] = args[i];
    }
    //fprintf(stderr, "about to do dot request %s\n", dr->name);
    cc_argc = dr->n_arguments;
    GraceObject *ret = do_request(context, receiver, dr->name);
    if (!ret) {
        fprintf(stderr, "Error: request %s returned null on receiver %p\n", dr->name, (void *)receiver);
        exit(1);
    }
    ref_dec(receiver);
    // for (int i = 0; i < dr->n_arguments; i++) {
    //     ref_dec(args[i]);
    // }
    return ret;
}

GraceObject *evaluateLexRequest(LexReqNode *dr, Context *context) {
    // fprintf(stderr, "evaluating lexreq %s\n", dr->name);
    if (strcmp(dr->name, "self") == 0) {
        return context->self;
    }
    GraceObject *receiver = findReceiver(context, dr->name);

    if (GC_DEBUG) fprintf(stderr, "Got receiver %p for %s\n", receiver, dr->name);
    if (!receiver) {
        fprintf(stderr, "Error: could not find receiver for %s\n", dr->name);
        debugFindReceiver(context, dr->name);
        exit(1);
    }
    // TODO: this is a leak
    // ref_inc(receiver);
    //fprintf(stderr, "got receiver %x\n", receiver);
    //fprintf(stderr, "arguments to find: %i\n", dr->n_arguments);
    GraceObject *args[dr->n_arguments];
    for (int i = 0; i < dr->n_arguments; i++) {
        //fprintf(stderr, "evaluating argument %i\n", i);
        args[i] = evaluate(dr->arguments[i], context);
        ref_inc(args[i]);
    }
    for (int i = 0; i < dr->n_arguments; i++) {
        cc_args[i] = args[i];
        args[i]->refcount--; // soft dec so isn't freed yet
    }
    cc_argc = dr->n_arguments;
    //fprintf(stderr, "about to do dot request %s\n", dr->name);
    GraceObject *ret = do_request(context, receiver, dr->name);
    //if (ret != receiver) ref_dec(receiver);
    return ret;
}

GraceObject *evaluateInterpStr(InterpStrNode *is, Context *context) {
    int startLen = strlen(is->start);
    GraceObject *middle = evaluate(is->middle, context);
    ref_inc(middle);
    GraceObject *after = evaluate(is->after, context);
    ref_inc(after);

    GraceObject *middleStr = request(context, middle, "asString", 0);
    ref_inc(middleStr);
    GraceObject *afterStr = request(context, after, "asString", 0);
    ref_inc(afterStr);

    struct GraceStringData *md = middleStr->data;
    struct GraceStringData *ad = afterStr->data;
    char *buf = malloc(startLen + md->length + ad->length + 1);
    strcpy(buf, is->start);
    strcpy(buf + startLen, md->str);
    strcpy(buf + startLen + md->length, ad->str);
    buf[startLen + md->length + ad->length] = 0;

    GraceObject *ret = graceString(buf);
    free(buf);
    ref_dec(afterStr);
    ref_dec(middleStr);
    ref_dec(middle);
    ref_dec(after);
    return ret;
}

GraceObject *evaluateAssign(AssignNode *an, Context *context) {
    GraceObject *receiver;
    char *name;
    if (an->lhs->nodetype == 'r') {
        // Dot request assignment
        DotReqNode *dr = (DotReqNode*)an->lhs;
        receiver = evaluate(dr->receiver, context);
        name = dr->name;
    } else if (an->lhs->nodetype == 'l') {
        LexReqNode *lr = (LexReqNode*)an->lhs;
        receiver = findReceiver(context, lr->name);
        name = lr->name;
        // fprintf(stderr, "found receiver %p for assignment to %s\n", (void *)receiver, lr->name);
        if (!receiver) {
            fprintf(stderr, "Error: could not find receiver for assignment to %s\n", lr->name);
            debugFindReceiver(context, lr->name);
            exit(1);
        }
    }
    ref_inc(receiver);
    GraceObject *value = evaluate(an->rhs, context);
    // fprintf(stderr, "evaluated rhs to %i %p\n", value->origin, (void *)value);
    cc_args[0] = value;
    cc_argc = 1;
    char setter_name[strlen(name) + 5];
    strcpy(setter_name, name);
    strcat(setter_name, ":=(1)");
    GraceObject *ret = do_request(context, receiver, setter_name);
    ref_dec(receiver);
    return ret;
}

GraceObject *evaluateReturn(ReturnNode *rn, Context *context) {
    GraceObject *value = evaluate(rn->expr, context);
    nonlocal_return_value = value;
    longjmp(*(context->return_buf), 1);
    return 0; // never reached
}

GraceObject *evaluateBlock(BlockNode *bn, Context *context) {
    return graceBlock(bn, context);
}

GraceObject *evaluateImport(ImportNode *in, Context *context) {
    struct ObjectMember *field = findField(context, in->as_name);
    if (!field) {
        fprintf(stderr, "Error: could not find field for import %s\n", in->as_name);
        exit(1);
    }
    struct ImportRecord *rec = imported_modules;
    while (rec) {
        if (strcmp(rec->path, in->path) == 0) {
            field->data = rec->module;
            ref_inc(rec->module);
            return Done;
        }
        rec = rec->next;
    }
    fprintf(stderr, "Error: no built-in import '%s' and no file import support yet\n", in->path);
    exit(1);
}

GraceObject *evaluate(ASTNode *node, Context *context) {
    switch(node->nodetype) {
        case 'n':
            return evaluateNumber((NumberNode*)node, context);
        case 's':
            return evaluateStrLit((StringNode*)node, context);
        case 'o':
            return evaluateObject((ObjectNode*)node, context);
        case 'v':
            return evaluateVar((VarNode*)node, context);
        case 'd':
            return evaluateDef((DefNode*)node, context);
        case '#':
            return Done;
        case 'r':
            return evaluateDotRequest((DotReqNode*)node, context);
        case 'l':
            return evaluateLexRequest((LexReqNode*)node, context);
        case 'S':
            return evaluateInterpStr((InterpStrNode*)node, context);
        case 'm':
            // Method declarations are no-ops when evaluated
            return Done;
        case '=':
            return evaluateAssign((AssignNode*)node, context);
        case 'R':
            return evaluateReturn((ReturnNode*)node, context);
        case 'b':
            return evaluateBlock((BlockNode*)node, context);
        case 'I':
            return evaluateImport((ImportNode*)node, context);
    }
    fprintf(stderr, "Error: could not evaluate node type %c\n", node->nodetype);
    exit(1);
    return 0;
}

GraceObject *prelude;

GraceObject *prelude_print(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *arg = cc_args[0];
    GraceObject *str = arg;
    if (GC_DEBUG) fprintf(stderr, "print called with %p of type %i\n", arg, arg->origin);
    if (arg->origin != OBJECT_STRING) {
        str = request(ctx, arg, "asString", 0);
    }
    struct GraceStringData *sd = str->data;
    printf("%s\n", sd->str);

    ref_discard(str);
    return Done;
}

GraceObject *prelude_done(GraceObject *self, int memberIndex, Context *ctx) {
    return Done;
}

GraceObject *prelude_const(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *ret = self->members[memberIndex].data;
    return ret;
}

GraceObject *prelude_if(GraceObject *self, int memberIndex, Context *ctx) {
    int num_args = cc_argc;
    for (int i = 0; i < num_args; i += 2) {
        GraceObject *cond = cc_args[i];
        GraceObject *thenPart = cc_args[i+1];
        if (i > 0) {
            if (cond->origin != OBJECT_BLOCK) {
                fprintf(stderr, "Error: elseif condition %i is not a block\n", i/2);
                exit(1);
            }
            cond = request(ctx, cond, "apply", 0);
            // ref_dec(cc_args[i]);
            // cc_args[i] = cond;
        }
        if (cond->origin != OBJECT_BOOLEAN) {
            fprintf(stderr, "Error: if condition %i is not a boolean\n", i/2);
            exit(1);
        }
        // fprintf(stderr, "if condition is %d; thenPart is %p %i\n", (int)cond->data, (void *)thenPart, thenPart->origin);
        GraceObject *block = 0;
        if (cond->data) {
            block = thenPart;
            ref_discard(cond);
            request(ctx, block, "apply", 0);
            return Done;
        }
        ref_discard(cond);
    }
    return Done;
}

GraceObject *prelude_if_else(GraceObject *self, int memberIndex, Context *ctx) {
    int num_args = cc_argc - 1;
    for (int i = 0; i < num_args; i += 2) {
        GraceObject *cond = cc_args[i];
        GraceObject *thenPart = cc_args[i+1];
        if (i > 0) {
            if (cond->origin != OBJECT_BLOCK) {
                fprintf(stderr, "Error: elseif condition %i is not a block\n", i/2);
                exit(1);
            }
            cond = request(ctx, cond, "apply", 0);
        }
        if (cond->origin != OBJECT_BOOLEAN) {
            fprintf(stderr, "Error: if condition %i is not a boolean\n", i/2);
            dump_object(cond);
            exit(1);
        }
        // fprintf(stderr, "if condition is %d; thenPart is %p %i\n", (int)cond->data, (void *)thenPart, thenPart->origin);
        GraceObject *block = 0;
        if (cond->data) {
            ref_discard(cond);
            block = thenPart;
            return request(ctx, block, "apply", 0);
        }
        ref_discard(cond);
    }
    GraceObject *elseBlock = cc_args[num_args];
    return request(ctx, elseBlock, "apply", 0);
}

GraceObject *prelude_while(GraceObject *self, int memberIndex, Context *ctx) {
    GraceObject *condBlock = cc_args[0];
    GraceObject *bodyBlock = cc_args[1];
    if (condBlock->origin != OBJECT_BLOCK) {
        fprintf(stderr, "Error: while condition is not a block\n");
        exit(1);
    }
    if (bodyBlock->origin != OBJECT_BLOCK) {
        fprintf(stderr, "Error: while body is not a block\n");
        exit(1);
    }
    while (1) {
        GraceObject *cond = request(ctx, condBlock, "apply", 0);
        if (cond->origin != OBJECT_BOOLEAN) {
            fprintf(stderr, "Error: while condition did not return a boolean\n");
            exit(1);
        }
        int result = cond->data ? 1 : 0;
        ref_discard(cond);
        if (!result) {
            break;
        }
        request(ctx, bodyBlock, "apply", 0);
    }
    return Done;
}

GraceObject *make_prelude() {
    GraceObject *ret = alloc_userobject(15);
    ret->members[0].name = "print(1)";
    ret->members[0].func = prelude_print;
    ret->members[1].name = "done";
    ret->members[1].func = prelude_done;
    ret->members[2].name = "true";
    ret->members[2].func = prelude_const;
    ret->members[2].data = graceBool(1);
    ref_inc(ret->members[2].data);
    ret->members[3].name = "false";
    ret->members[3].func = prelude_const;
    ret->members[3].data = graceBool(0);
    ref_inc(ret->members[3].data);
    ret->members[4].name = "if(1)then(1)";
    ret->members[4].func = prelude_if;
    ret->members[5].name = "if(1)then(1)else(1)";
    ret->members[5].func = prelude_if_else;
    ret->members[6].name = "if(1)then(1)elseif(1)then(1)";
    ret->members[6].func = prelude_if;
    ret->members[7].name = "if(1)then(1)elseif(1)then(1)elseif(1)then(1)";
    ret->members[7].func = prelude_if;
    ret->members[8].name = "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)";
    ret->members[8].func = prelude_if;
    ret->members[9].name = "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)";
    ret->members[9].func = prelude_if;
    ret->members[10].name = "if(1)then(1)elseif(1)then(1)else(1)";
    ret->members[10].func = prelude_if_else;
    ret->members[11].name = "if(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)";
    ret->members[11].func = prelude_if_else;
    ret->members[12].name = "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)";
    ret->members[12].func = prelude_if_else;
    ret->members[13].name = "if(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)elseif(1)then(1)else(1)";
    ret->members[13].func = prelude_if_else;
    ret->members[14].name = "while(1)do(1)";
    ret->members[14].func = prelude_while;
    ret->origin = OBJECT_PRELUDE;
    return ret;
}

void dump_object(GraceObject *obj) {
    fprintf(stderr, "Dumping object at %p:\n", obj);
    fprintf(stderr, "  Origin:           \t%i", obj->origin);
    switch(obj->origin) {
        case OBJECT_USER:    fprintf(stderr, " (user object)\n"); break;
        case OBJECT_NUMBER:  fprintf(stderr, " (number)\n"); break;
        case OBJECT_STRING:  fprintf(stderr, " (string)\n"); break;
        case OBJECT_PRELUDE: fprintf(stderr, " (prelude)\n"); break;
        case OBJECT_DONE:    fprintf(stderr, " (done)\n"); break;
        case OBJECT_SCOPE:   fprintf(stderr, " (scope)\n"); break;
        case OBJECT_BOOLEAN: fprintf(stderr, " (boolean)\n"); break;
        case OBJECT_BLOCK:   fprintf(stderr, " (block)\n"); break;
        default:             fprintf(stderr, " (unknown)\n"); break;
    }
    fprintf(stderr, "  Flags:           \t%i\n", obj->flags);
    fprintf(stderr, "  Refcount:        \t%i\n", obj->refcount);
    if (obj->origin == OBJECT_BOOLEAN) {
        fprintf(stderr, "  Value:\tboolean %s\n", obj->data ? "true" : "false");
    } else if (obj->origin == OBJECT_BLOCK) {
        Context *ctx = obj->data;
        fprintf(stderr, "  Scope: %p\n", (void *)ctx->scope);
        fprintf(stderr, "  Self: %p\n", (void *)ctx->self);
    } else if (obj->origin == OBJECT_STRING) {
        struct GraceStringData *sd = obj->data;
        if (sd->length > 30) {
            fprintf(stderr, "  Value:\tstring %i \"%.*s...\"\n", sd->length, 20, sd->str);
        } else {
            fprintf(stderr, "  Value:\tstring %i \"%s\"\n", sd->length, sd->str);
        }
    } else if (obj->origin == OBJECT_NUMBER) {
        fprintf(stderr, "  Value:\tnumber %f\n", *((double*)obj->data));
    }

    fprintf(stderr, "  Number of members:\t%i\n", obj->n_members);
    for (int i = 0; i < obj->n_members; i++) {
        fprintf(stderr, "  Member %i\n", i);
        fprintf(stderr, "    Name:\t%s\n", obj->members[i].name);
        if (obj->members[i].flags & FLAG_FIELD) {
            GraceObject *val = obj->members[i].data;
            if (val->origin == OBJECT_NUMBER) {
                fprintf(stderr, "    Value:\tnumber %f\n", *((double*)val->data));
            } else if (val->origin == OBJECT_STRING) {
                struct GraceStringData *sd = val->data;
                if (sd->length > 30) {
                    fprintf(stderr, "    Value:\tstring %i \"%.*s...\"\n", sd->length, 20, sd->str);
                } else {
                    fprintf(stderr, "    Value:\tstring %i \"%s\"\n", sd->length, sd->str);
                }
            } else {
                fprintf(stderr, "    Value:\t0x%p\n", val);
            }
        }
        if (obj->members[i].flags & FLAG_WRITER) {
            struct ObjectMember *other = obj->members[i].data;
            fprintf(stderr, "    Setter for:\t%s\n", other->name);
        }
    }
    fprintf(stderr, "--end\n");
}


int grace_setup() {
    if (getenv("GC_DEBUG"))
        GC_DEBUG = 1;
    if (getenv("GC_DISABLE"))
        GC_DISABLE = 1;
    Done = alloc_object(1, OBJECT_DONE);
    Done->flags = FLAG_IMMORTAL | FLAG_LIVING;
    Done->members[0].name = "asString";
    Done->members[0].func = GraceObject__const_asString;
    Done->members[0].data = "<done>";

    prelude = make_prelude();
    prelude->flags |= FLAG_IMMORTAL;

    if (GC_DEBUG) fprintf(stderr, "Done is %p. Prelude is %p\n", (void *)Done, (void *)prelude);
    return 0;
}

int grace_teardown(int with_diagnostics) {
    graceBool__false->refcount = 0;
    graceBool__false->flags &= ~FLAG_IMMORTAL;
    graceBool__true->refcount = 0;
    graceBool__true->flags &= ~FLAG_IMMORTAL;
    Done->refcount = 0;
    Done->flags &= ~FLAG_IMMORTAL;
    prelude->refcount = 0;
    prelude->flags &= ~FLAG_IMMORTAL;
    ref_discard(graceBool__true);
    ref_discard(graceBool__false);
    ref_discard(Done);
    ref_discard(prelude);
    if (with_diagnostics && allocated_objects != freed_objects || GC_DEBUG)
        fprintf(stderr, "Allocated: %i  Freed: %i\n", allocated_objects, freed_objects);
    if (GC_DEBUG) {
        if (allocated_objects != freed_objects) {
            fprintf(stderr, "%i uncollected objects:\n", allocated_objects - freed_objects);
            struct AllocatedObject *obj = allocated_objects_list;
            while (obj) {
                if (!(obj->obj->flags & FLAG_FREED))
                    dump_object(obj->obj);
                obj = obj->next;
            }
        }
    }
    return 0;
}

#ifndef GRACE_RUNTIME_NO_MAIN
int main(int argc, char **argv) {
    grace_setup();
    Context ctx;
    ctx.scope = prelude;
    ctx.self = prelude;

    program = objCons(cons(defDec("name", nil, nil, strLit("world")), one(lexReq(one(part("print", one(interpStr("Hello, ", lexReq(one(part("name", nil))), strLit(safeStr("", charExclam, ""))))))))), nil);

    GraceObject *module = evaluate(program, &ctx);
    grace_teardown(0);
    return 0;
}
#endif
