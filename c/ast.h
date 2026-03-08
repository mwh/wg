/* Declares prototypes for the AST construction functions,
 * shared by the baked Grace source files.
 *
 * Each concise node kind is mapped to a short C identifier:
 *
 *   o0C(body, anns)              objectConstructor
 *   n0M(val)                     numberNode   (double)
 *   s0L(val)                     stringNode   (const char*)
 *   b1K(params, stmts)           block
 *   d3F(name, dtype, anns, val)  defDecl
 *   v4R(name, dtype, anns, val)  varDecl
 *   m0D(parts, ret, anns, body)  methodDecl
 *   p0T(name, params, gen)       part
 *   l0R(name, args, gen)         lexicalRequest
 *   d0R(recv, name, args, gen)   explicitRequest (dotRequest)
 *   a5N(left, right)             assign
 *   r3T(val)                     returnStmt
 *   i0D(name, dtype)             identifierDeclaration
 *   i0S(pre, expr, rest)         interpString
 *   i0M(src, binding)            importStmt
 *   d0S(src)                     dialectStmt
 *   c0M(text)                    comment
 *   i0C(body)                    interfaceConstructor
 *   m0S(parts, ret)              methSig
 *   t0D(name, gen, val)          typeDecl
 *   s4F(a, b, c)                 string concat of three strings (returns const char*)
 *   c0N(head, tail)              cons list node  (head may be ASTNode* or const char*)
 *   c2N(a, b)                    cons(a, cons(b, nil))
 *   o1N(x)                       cons(x, nil)    (x may be ASTNode* or const char*)
 *   nil                          empty list (NULL)
 */

#ifndef AST_H
#define AST_H

/*  Node kind codes  */
#define NK_OBJCONS      1
#define NK_NUMLIT       2
#define NK_STRLIT       3
#define NK_LEXREQ       4
#define NK_DOTREQ       5
#define NK_ASSIGN       6
#define NK_IDENT_DECL   7
#define NK_RETURN_STMT  8
#define NK_PART         9
#define NK_DEF_DECL    10
#define NK_VAR_DECL    11
#define NK_METH_DECL   12
#define NK_BLOCK       13
#define NK_INTERP_STR  14
#define NK_IMPORT_STMT 15
#define NK_DIALECT_STMT 16
#define NK_COMMENT     17
#define NK_TYPE_DECL   18
#define NK_IFACE_CONS  19
#define NK_METH_SIG    20
#define NK_CONS        21
/* NK_NIL is represented by the NULL pointer */

/*  ASTNode  */
typedef struct ASTNode ASTNode;
struct ASTNode {
    int kind;
    double numval;          /* NK_NUMLIT */
    const char *strval;     /* NK_STRLIT, NK_STRLIT-based fields (names, texts) */
    ASTNode *a1;            /* varies by kind - see table above */
    ASTNode *a2;
    ASTNode *a3;
    ASTNode *a4;
};

/* nil list - public, defined in grace.c */
extern ASTNode *nil;

/*  Special string constants (single Unicode chars as C strings)  */
extern const char *c9Q;   /* "    */
extern const char *c9B;   /* \    */
extern const char *c9N;   /* \n   */
extern const char *c9R;   /* \r   */
extern const char *c9S;   /* *    */
extern const char *c9M;   /* &    */
extern const char *c9E;   /* !    */
extern const char *c9P;   /* %    */
extern const char *c9A;   /* @    */
extern const char *c9L;   /* {    */
extern const char *c9D;   /* $    */
extern const char *c9G;   /* `    */
extern const char *c9T;   /* ~    */
extern const char *c9C;   /* ^    */
extern const char *c9H;   /* #    */

/*  String concatenation (used in the baked files as s4F(a,b,c))  */
const char *s4F(const char *a, const char *b, const char *c);

/*  List constructors 
 * o1N and c0N accept either ASTNode* or const char* heads.
 * When a const char* is passed it is automatically wrapped in s0L().
 */
ASTNode *_o1N_node(ASTNode *x);
ASTNode *_o1N_str(const char *s);
ASTNode *_c0N_node(ASTNode *head, ASTNode *tail);
ASTNode *_c0N_str(const char *head, ASTNode *tail);

#define o1N(x) _Generic((x),           \
    ASTNode *:    _o1N_node,            \
    const char *: _o1N_str,             \
    char *:       _o1N_str              \
)(x)

#define c0N(h, t) _Generic((h),         \
    ASTNode *:    _c0N_node,            \
    const char *: _c0N_str,             \
    char *:       _c0N_str              \
)((h),(t))

ASTNode *c2N(ASTNode *a, ASTNode *b);

/*  Node constructors  */

/* o0C  objectConstructor(body, annotations) */
ASTNode *o0C(ASTNode *body, ASTNode *anns);

/* n0M  numberNode(value) */
ASTNode *n0M(double val);

/* s0L  stringNode(value) */
ASTNode *s0L(const char *val);

/* b1K  block(params, statements) */
ASTNode *b1K(ASTNode *params, ASTNode *stmts);

/* d3F  defDecl(name, decType, annotations, value) */
ASTNode *d3F(const char *name, ASTNode *dtype, ASTNode *anns, ASTNode *val);

/* v4R  varDecl(name, decType, annotations, initialValue-or-nil) */
ASTNode *v4R(const char *name, ASTNode *dtype, ASTNode *anns, ASTNode *val);

/* m0D  methodDecl(parts, returnType, annotations, body) */
ASTNode *m0D(ASTNode *parts, ASTNode *rettype, ASTNode *anns, ASTNode *body);

/* p0T  part(name, params, genericParams) */
ASTNode *p0T(const char *name, ASTNode *params, ASTNode *gen);

/* l0R  lexicalRequest(name-with-arity, args, genericArgs) */
ASTNode *l0R(const char *name, ASTNode *args, ASTNode *gen);

/* d0R  explicitRequest(receiver, name-with-arity, args, genericArgs) */
ASTNode *d0R(ASTNode *recv, const char *name, ASTNode *args, ASTNode *gen);

/* a5N  assign(left, right) */
ASTNode *a5N(ASTNode *left, ASTNode *right);

/* r3T  returnStmt(value) */
ASTNode *r3T(ASTNode *val);

/* i0D  identifierDeclaration(name, decType) */
ASTNode *i0D(const char *name, ASTNode *dtype);

/* i0S  interpString(pre-string, expression, rest) */
ASTNode *i0S(const char *pre, ASTNode *expr, ASTNode *rest);

/* i0M  importStmt(source-path, binding-identDecl) */
ASTNode *i0M(const char *src, ASTNode *binding);

/* d0S  dialectStmt(source-path) */
ASTNode *d0S(const char *src);

/* c0M  comment(text) */
ASTNode *c0M(const char *text);

/* i0C  interfaceConstructor(body) */
ASTNode *i0C(ASTNode *body);

/* m0S  methSig(parts, returnType) */
ASTNode *m0S(ASTNode *parts, ASTNode *rettype);

/* t0D  typeDecl(name, genericParams, value) */
ASTNode *t0D(const char *name, ASTNode *gen, ASTNode *val);

#endif /* AST_H */
