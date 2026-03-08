/* main.c - entry point for the Grace CPS interpreter */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "grace.h"
#include "gc.h"

/* ASTNode* for the three baked modules */
extern ASTNode *ast_ast;
extern ASTNode *lexer_ast;
extern ASTNode *parser_ast;

/*  Read a whole file into a malloc'd buffer  */
static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "grace: cannot open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fprintf(stderr, "grace: out of memory\n"); exit(1); }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf;
}

/*  Evaluate a baked ASTNode* tree, returning the resulting object  */
static GraceObject *eval_baked(ASTNode *ast, Env *env) {
    CaptureCont cc;
    cc.base.apply = capture_apply;
    cc.base.gc_trace = NULL;  /* stack-local, no GC trace needed */
    cc.base.cleanup = NULL;
    cc.base.refcount = CONT_REFCOUNT_STATIC;
    cc.base.consumed = 0;
    cc.result     = grace_done;
    trampoline(eval_node(ast, env, (Cont *)&cc));
    return cc.result;
}

/*  main  */
int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: grace <file.grace>\n");
        return 1;
    }
    const char *user_file = argv[1];
    char *source = read_file(user_file);

    /*  1. Build prelude  */
    GraceObject *prelude = make_prelude();
    gc_push_root(&prelude);

    /*  2. Outer environment   */
    Env *env = env_new(prelude);

    /*  3. Evaluate baked/ast.c - creates the 'ast' factory object  */
    GraceObject *ast_obj = eval_baked(ast_ast, env);
    grace_register_module("ast", ast_obj);
    /* Also expose in prelude scope */
    user_bind_def(prelude, "ast", ast_obj);

    /*  4. Evaluate baked/lexer.c  */
    GraceObject *lexer_obj = eval_baked(lexer_ast, env);
    grace_register_module("lexer", lexer_obj);
    user_bind_def(prelude, "lexer", lexer_obj);

    /*  5. Evaluate baked/parser.c  */
    GraceObject *parser_obj = eval_baked(parser_ast, env);
    grace_register_module("parser", parser_obj);

    /*  6. Parse the user file  */
    GraceObject *parse_args[2] = {
        grace_string_new(user_file),
        grace_string_new(source)
    };
    // puts("Parsing...");
    GraceObject *grace_ast_obj = grace_request_sync(parser_obj, env,
                                                     "parseModule(2)",
                                                     parse_args, 2);
    // puts("Parsing done. Converting...");
    /*  7. Convert Grace AST object -> ASTNode* tree  */
    ASTNode *program = grace_ast_to_astnode(grace_ast_obj, env);
    if (!program) {
        fprintf(stderr, "gracei: parser returned nothing for '%s'\n", user_file);
        return 1;
    }

    // puts("Conversion done. Evaluating...");
    /*  8. Evaluate the user program  */
    /* The top-level is an object constructor; eval produces a GraceObject.
     * Final result is ignored, but side effects still happen.
     */
    //log_requests = 1;
    CaptureCont result_cc;
    result_cc.base.apply = capture_apply;
    result_cc.base.gc_trace = NULL;
    result_cc.base.cleanup = NULL;
    result_cc.base.refcount = CONT_REFCOUNT_STATIC;
    result_cc.base.consumed = 0;
    result_cc.result     = grace_done;
    trampoline(eval_node(program, env, (Cont *)&result_cc));

    free(source);
    return 0;
}
