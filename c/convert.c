/* Convert in-Grace AST objects back to ASTNode* trees */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "grace.h"

/*  Synchronous field access  */
static GraceObject *get_field(GraceObject *obj, Env *env, const char *fname) {
    char *mname = str_fmt("%s(0)", fname);
    GraceObject *result = grace_request_sync(obj, env, mname, NULL, 0);
    free(mname);
    return result;
}

/*  Check whether a Grace object represents "nil" (grace_done or a 
 *     grace_user_object that has an "end(0)" returning true)  */
static int grace_is_nil(GraceObject *obj, Env *env) {
    if (!obj || obj == grace_done) return 1;
    /* For list objects: check "end(0)" - but only if the object actually has it.
     * Scan the own method list without calling. */
    if (obj->vt == &grace_user_vtable) {
        GraceUserObject *uo = (GraceUserObject *)obj;
        int has_end = 0;
        for (MethodEntry *m = uo->methods; m; m = m->next)
            if (strcmp(m->name, "end(0)") == 0) { has_end = 1; break; }
        if (!has_end) return 0;  /* Not a list object */
        GraceObject *r = grace_request_sync(obj, env, "end(0)", NULL, 0);
        if (r && r->vt == &grace_bool_vtable && grace_bool_val(r)) return 1;
    }
    return 0;
}

/* Count Grace list length synchronously */
static int count_grace_list(GraceObject *list, Env *env) {
    int n = 0;
    GraceObject *cur = list;
    while (cur && !grace_is_nil(cur, env)) {
        n++;
        cur = get_field(cur, env, "tail");
        if (!cur) break;
    }
    return n;
}

/*  Convert a Grace list to a flat ASTNode* cons list  */
static ASTNode *convert_list(GraceObject *list, Env *env);

ASTNode *grace_ast_to_astnode(GraceObject *obj, Env *env);

/* Collect grace list elements into an ASTNode* cons list */
static ASTNode *convert_list(GraceObject *list, Env *env) {
    if (grace_is_nil(list, env)) return NULL;
    GraceObject *head_obj = get_field(list, env, "head");
    GraceObject *tail_obj = get_field(list, env, "tail");
    ASTNode *head = grace_ast_to_astnode(head_obj, env);
    ASTNode *tail = convert_list(tail_obj, env);
    return _c0N_node(head, tail);  /* single cons cell: cons(head, tail) */
}

/*  Get string field value  */
static const char *get_str_field(GraceObject *obj, Env *env, const char *fname) {
    GraceObject *v = get_field(obj, env, fname);
    if (!v || v == grace_done) return NULL;
    if (v->vt == &grace_string_vtable) return str_dup(grace_string_val(v));
    /* Try asString */
    GraceObject *s = grace_request_sync(v, env, "asString(0)", NULL, 0);
    if (s && s->vt == &grace_string_vtable) return str_dup(grace_string_val(s));
    return NULL;
}

/*  Get number field value  */
static double get_num_field(GraceObject *obj, Env *env, const char *fname) {
    GraceObject *v = get_field(obj, env, fname);
    if (!v || v == grace_done) return 0.0;
    if (v->vt == &grace_number_vtable) return grace_number_val(v);
    return 0.0;
}

/* Build flat method name from a Grace "parts" list */
/* The parts list is: cons(part1, cons(part2, nil))
 * Each part has: name(0) -> string, parameters(0) -> list
 */
static char *build_name_from_parts(GraceObject *parts, Env *env) {
    char *name = str_dup("");
    GraceObject *cur = parts;
    while (cur && !grace_is_nil(cur, env)) {
        GraceObject *part     = get_field(cur, env, "head");
        const char *pname     = get_str_field(part, env, "name");
        GraceObject *params   = get_field(part, env, "parameters");
        int nparam = count_grace_list(params, env);
        char *seg = str_fmt("%s(%d)", pname ? pname : "", nparam);
        if (pname) free((void *)pname);
        char *tmp = str_cat(name, seg);
        free(seg); free(name);
        name = tmp;
        cur = get_field(cur, env, "tail");
    }
    return name;
}

/*  Flatten args from parts list into an ASTNode* cons list  */
static ASTNode *flatten_args_from_parts(GraceObject *parts, Env *env) {
    /* Collect all params from all parts into one flat list
     * First, collect all param ASTNode* in an array
     * Walk twice: once to count, once to collect
     * For simplicity, build a linked list using a tail pointer */
    ASTNode *head = NULL, **tail = &head;

    GraceObject *cur = parts;
    while (cur && !grace_is_nil(cur, env)) {
        GraceObject *part   = get_field(cur, env, "head");
        GraceObject *params = get_field(part, env, "parameters");
        GraceObject *pc = params;
        while (pc && !grace_is_nil(pc, env)) {
            GraceObject *param_obj = get_field(pc, env, "head");
            ASTNode *param_node = grace_ast_to_astnode(param_obj, env);
            ASTNode *cell = _o1N_node(param_node);  /* cons(param, NULL) */
            *tail = cell;
            tail  = &cell->a2;
            pc = get_field(pc, env, "tail");
        }
        cur = get_field(cur, env, "tail");
    }
    return head;
}

/*  Flatten generic params from parts list  */
static ASTNode *flatten_generics_from_parts(GraceObject *parts, Env *env) {
    ASTNode *head = NULL, **tail = &head;
    GraceObject *cur = parts;
    while (cur && !grace_is_nil(cur, env)) {
        GraceObject *part       = get_field(cur, env, "head");
        GraceObject *generics_o = grace_request_sync(part, env, "genericParameters(0)", NULL, 0);
        if (generics_o && !grace_is_nil(generics_o, env)) {
            GraceObject *gc = generics_o;
            while (gc && !grace_is_nil(gc, env)) {
                GraceObject *gp = get_field(gc, env, "head");
                ASTNode *gn = grace_ast_to_astnode(gp, env);
                ASTNode *cell = _o1N_node(gn);  /* cons(gn, NULL) */
                *tail = cell;
                tail  = &cell->a2;
                gc = get_field(gc, env, "tail");
            }
        }
        cur = get_field(cur, env, "tail");
    }
    return head;
}

/*  Main conversion  */
ASTNode *grace_ast_to_astnode(GraceObject *obj, Env *env) {
    if (!obj || obj == grace_done || obj == grace_uninit) return NULL;
    if (grace_is_nil(obj, env)) return NULL;

    /* If the object is already a plain Grace string (e.g. annotation like "public"),
       return it as a string-literal AST node rather than trying to call kind(0). */
    if (obj->vt == &grace_string_vtable) {
        return s0L(str_dup(grace_string_val(obj)));
    }

    /* Get the kind */
    GraceObject *kind_obj = grace_request_sync(obj, env, "kind(0)", NULL, 0);
    if (!kind_obj || kind_obj->vt != &grace_string_vtable) return NULL;
    const char *kind = grace_string_val(kind_obj);

    /* Nil / cons lists */
    if (strcmp(kind, "nil") == 0) return NULL;

    if (strcmp(kind, "cons") == 0) {
        /* This is a list cell; convert as a list */
        GraceObject *head_obj = get_field(obj, env, "head");
        GraceObject *tail_obj = get_field(obj, env, "tail");
        ASTNode *head = grace_ast_to_astnode(head_obj, env);
        ASTNode *tail = grace_ast_to_astnode(tail_obj, env);
        return c2N(head, tail);
    }

    /* Numeric literal */
    if (strcmp(kind, "numLit") == 0) {
        double v = get_num_field(obj, env, "value");
        return n0M(v);
    }

    /* String literal */
    if (strcmp(kind, "strLit") == 0) {
        const char *v = get_str_field(obj, env, "value");
        return s0L(v ? v : "");
    }

    /* Interpolated string */
    if (strcmp(kind, "interpStr") == 0) {
        const char *prefix = get_str_field(obj, env, "value");
        GraceObject *expr_obj = get_field(obj, env, "expression");
        GraceObject *next_obj = get_field(obj, env, "next");
        ASTNode *expr = grace_ast_to_astnode(expr_obj, env);
        ASTNode *next = grace_ast_to_astnode(next_obj, env);
        return i0S(prefix ? prefix : "", expr, next);
    }

    /* Block */
    if (strcmp(kind, "block") == 0) {
        GraceObject *params_obj = get_field(obj, env, "parameters");
        GraceObject *body_obj   = get_field(obj, env, "statements"); /* Grace AST uses "statements" */
        ASTNode *params = convert_list(params_obj, env);
        ASTNode *body   = convert_list(body_obj,   env);
        return b1K(params, body);
    }

    /* Object constructor */
    if (strcmp(kind, "objCons") == 0) {
        GraceObject *body_obj = get_field(obj, env, "body");
        GraceObject *anns_obj = get_field(obj, env, "annotations");
        ASTNode *body = convert_list(body_obj, env);
        ASTNode *anns = convert_list(anns_obj, env);
        return o0C(body, anns);
    }

    /* Method declaration */
    if (strcmp(kind, "methDec") == 0) {
        GraceObject *parts_obj   = get_field(obj, env, "parts");
        GraceObject *ret_obj     = get_field(obj, env, "returnType");
        GraceObject *anns_obj    = get_field(obj, env, "annotations");
        GraceObject *body_obj    = get_field(obj, env, "body");
        ASTNode *parts = convert_list(parts_obj, env);
        ASTNode *ret   = grace_ast_to_astnode(ret_obj, env);
        ASTNode *anns  = convert_list(anns_obj, env);
        ASTNode *body  = convert_list(body_obj, env);
        return m0D(parts, ret, anns, body);
    }

    /* Method signature */
    if (strcmp(kind, "methSig") == 0) {
        GraceObject *parts_obj = get_field(obj, env, "parts");
        GraceObject *ret_obj   = get_field(obj, env, "returnType");
        ASTNode *parts = convert_list(parts_obj, env);
        ASTNode *ret   = grace_ast_to_astnode(ret_obj, env);
        return m0S(parts, ret);
    }

    /* Part */
    if (strcmp(kind, "part") == 0) {
        const char *name = get_str_field(obj, env, "name");
        GraceObject *params_obj  = get_field(obj, env, "parameters");
        GraceObject *generics_ob = grace_request_sync(obj, env, "genericParameters(0)", NULL, 0);
        ASTNode *params  = convert_list(params_obj,  env);
        ASTNode *generics = generics_ob ? convert_list(generics_ob, env) : NULL;
        return p0T(name ? name : "", params, generics);
    }

    /* Def declaration */
    if (strcmp(kind, "defDec") == 0) {
        const char *name     = get_str_field(obj, env, "name");
        GraceObject *type_ob = get_field(obj, env, "decType");
        GraceObject *anns_ob = get_field(obj, env, "annotations");
        GraceObject *val_ob  = get_field(obj, env, "value");
        ASTNode *decType = grace_ast_to_astnode(type_ob, env);
        ASTNode *anns    = convert_list(anns_ob, env);
        ASTNode *val     = grace_ast_to_astnode(val_ob, env);
        return d3F(name ? name : "", decType, anns, val);
    }

    /* Var declaration */
    if (strcmp(kind, "varDec") == 0) {
        const char *name     = get_str_field(obj, env, "name");
        GraceObject *type_ob = get_field(obj, env, "decType");
        GraceObject *anns_ob = get_field(obj, env, "annotations");
        GraceObject *val_ob  = get_field(obj, env, "value");
        ASTNode *decType = grace_ast_to_astnode(type_ob, env);
        ASTNode *anns    = convert_list(anns_ob, env);
        /* varDec.value is cons(initExpr, nil) when present, or nil when absent */
        ASTNode *val = NULL;
        if (!grace_is_nil(val_ob, env)) {
            GraceObject *val_head = get_field(val_ob, env, "head");
            val = grace_ast_to_astnode(val_head, env);
        }
        return v4R(name ? name : "", decType, anns, val);
    }

    /* Type declaration */
    if (strcmp(kind, "typeDec") == 0) {
        const char *name     = get_str_field(obj, env, "name");
        GraceObject *gen_ob  = get_field(obj, env, "genericParameters");
        GraceObject *val_ob  = get_field(obj, env, "value");
        ASTNode *generics = convert_list(gen_ob, env);
        ASTNode *val      = grace_ast_to_astnode(val_ob, env);
        return t0D(name ? name : "", generics, val);
    }

    /* Interface constructor */
    if (strcmp(kind, "interfaceCons") == 0 || strcmp(kind, "iface") == 0) {
        GraceObject *body_ob = get_field(obj, env, "body");
        ASTNode *body = convert_list(body_ob, env);
        return i0C(body);
    }

    /* Identifier declaration */
    if (strcmp(kind, "identifierDeclaration") == 0
     || strcmp(kind, "identDec") == 0) {
        const char *name     = get_str_field(obj, env, "name");
        GraceObject *type_ob = get_field(obj, env, "decType");
        ASTNode *decType = grace_ast_to_astnode(type_ob, env);
        return i0D(name ? name : "", decType);
    }

    /* Lexical request */
    if (strcmp(kind, "lexReq") == 0) {
        GraceObject *parts_obj = get_field(obj, env, "parts");
        char *name = build_name_from_parts(parts_obj, env);
        ASTNode *args     = flatten_args_from_parts(parts_obj, env);
        ASTNode *generics = flatten_generics_from_parts(parts_obj, env);
        return l0R(name, args, generics);
    }

    /* Dot/explicit request */
    if (strcmp(kind, "dotReq") == 0) {
        GraceObject *recv_obj  = get_field(obj, env, "receiver");
        GraceObject *parts_obj = get_field(obj, env, "parts");
        ASTNode *recv     = grace_ast_to_astnode(recv_obj, env);
        char *name        = build_name_from_parts(parts_obj, env);
        ASTNode *args     = flatten_args_from_parts(parts_obj, env);
        ASTNode *generics = flatten_generics_from_parts(parts_obj, env);
        return d0R(recv, name, args, generics);
    }

    /* Assignment */
    if (strcmp(kind, "assn") == 0 || strcmp(kind, "assign") == 0) {
        GraceObject *lhs_ob = get_field(obj, env, "left");
        GraceObject *rhs_ob = get_field(obj, env, "right");
        ASTNode *lhs = grace_ast_to_astnode(lhs_ob, env);
        ASTNode *rhs = grace_ast_to_astnode(rhs_ob, env);
        return a5N(lhs, rhs);
    }

    /* Return statement */
    if (strcmp(kind, "returnStmt") == 0 || strcmp(kind, "return") == 0) {
        GraceObject *val_ob = get_field(obj, env, "value");
        ASTNode *val = grace_ast_to_astnode(val_ob, env);
        return r3T(val);
    }

    /* Comment */
    if (strcmp(kind, "comment") == 0) {
        const char *v = get_str_field(obj, env, "value");
        return c0M(v ? v : "");
    }

    /* Import statement */
    if (strcmp(kind, "importStmt") == 0 || strcmp(kind, "import") == 0) {
        const char *src    = get_str_field(obj, env, "source");
        GraceObject *bind  = get_field(obj, env, "binding");
        ASTNode *binding   = grace_ast_to_astnode(bind, env);
        return i0M(src ? src : "", binding);
    }

    /* Dialect statement */
    if (strcmp(kind, "dialectStmt") == 0 || strcmp(kind, "dialect") == 0) {
        const char *src = get_str_field(obj, env, "source");
        return d0S(src ? src : "");
    }

    /*  Lineup  */
    if (strcmp(kind, "lineup") == 0) {
        GraceObject *elems_obj = get_field(obj, env, "elements");
        ASTNode *elems = convert_list(elems_obj, env);
        return l0N(elems);
    }

    /*  Inherit statement  */
    if (strcmp(kind, "inheritStmt") == 0) {
        GraceObject *parent_obj = get_field(obj, env, "parent");
        ASTNode *parent = grace_ast_to_astnode(parent_obj, env);
        return i0H(parent, NULL);
    }

    /*  Use statement  */
    if (strcmp(kind, "useStmt") == 0) {
        GraceObject *parent_obj = get_field(obj, env, "parent");
        ASTNode *parent = grace_ast_to_astnode(parent_obj, env);
        return u0S(parent, NULL);
    }

    /* Unknown kind - warn and return NULL */
    fprintf(stderr, "grace_ast_to_astnode: unknown kind '%s'\n", kind);
    return NULL;
}
