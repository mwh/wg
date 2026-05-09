#include <stdlib.h>
#include <string.h>
#include "grace.h"
#include "gc.h"


/* Fixed strings for token natures (interned) */
GraceObject *TK_NATURE_EOF;
GraceObject *TK_NATURE_NUMBER;
GraceObject *TK_NATURE_LPAREN;
GraceObject *TK_NATURE_RPAREN;
GraceObject *TK_NATURE_LGENERIC;
GraceObject *TK_NATURE_RGENERIC;
GraceObject *TK_NATURE_LBRACE;
GraceObject *TK_NATURE_RBRACE;
GraceObject *TK_NATURE_LSQUARE;
GraceObject *TK_NATURE_RSQUARE;
GraceObject *TK_NATURE_COMMA;
GraceObject *TK_NATURE_DOT;
GraceObject *TK_NATURE_IDENTIFIER;
GraceObject *TK_NATURE_KEYWORD;
GraceObject *TK_NATURE_OPERATOR;
GraceObject *TK_NATURE_STRING;
GraceObject *TK_NATURE_INTERPSTRING;
GraceObject *TK_NATURE_EQUALS;
GraceObject *TK_NATURE_ASSIGN;
GraceObject *TK_NATURE_ARROW;
GraceObject *TK_NATURE_NEWLINE;
GraceObject *TK_NATURE_COLON;
GraceObject *TK_NATURE_SEMICOLON;
GraceObject *TK_NATURE_COMMENT;
GraceObject *TK_NATURE_ERROR;

const GraceVTable lexer_object_vtable;

const char *lexer_modulePrefix = "unknown:";

/* Token objects: line, column, nature, value, +1 optional */
typedef struct {
    GraceObject base;
    GraceObject *line;
    GraceObject *column;
    GraceObject *nature;
    GraceObject *value;
    GraceObject *additional;  /* optional, may be NULL */
} LexerToken;

static PendingStep *token_request(GraceObject *self, Env *env,
                                      const char *name,
                                      GraceObject **args, int nargs, Cont *k) {
    (void)env;(void)args;(void)nargs;
    if (strcmp(name, "line(0)")==0) return cont_apply(k, ((LexerToken *)self)->line);
    if (strcmp(name, "column(0)")==0) return cont_apply(k, ((LexerToken *)self)->column);
    if (strcmp(name, "nature(0)")==0) return cont_apply(k, ((LexerToken *)self)->nature);
    if (strcmp(name, "value(0)")==0) return cont_apply(k, ((LexerToken *)self)->value);
    if (strcmp(name, "index(0)")==0) return cont_apply(k, ((LexerToken *)self)->additional);
    if (strcmp(name, "==(0)")==0) return cont_apply(k, grace_bool(self == args[0]));
    if (strcmp(name, "location(0)")==0) {
        LexerToken *t = (LexerToken *)self;
        int line = (int)grace_number_val(t->line);
        int col = (int)grace_number_val(t->column);
        return cont_apply(k, grace_string_new(str_fmt("%s%d:%d", lexer_modulePrefix, line, col)));
    }
    if (strcmp(name, "asString(0)")==0) {
        LexerToken *t = (LexerToken *)self;
        int line = (int)grace_number_val(t->line);
        int col  = (int)grace_number_val(t->column);
        const char *nature = grace_string_val(t->nature);
        if (t->value && t->value->vt == &grace_string_vtable) {
            nature = str_fmt("%s(%s)[%d:%d]", nature, grace_string_val(t->value),
            line, col);
        } else {
            nature = str_fmt("%s[%d:%d]", nature, line, col);
        }
        return cont_apply(k, grace_string_new(nature));
    }
    grace_fatal("No method '%s' on LexerToken", name);
}

static const char *token_describe(GraceObject *self) { (void)self; return "a token"; }

static void token_trace(GraceObject *self) {
    LexerToken *t = (LexerToken *)self;
    gc_mark_grey(t->line);
    gc_mark_grey(t->column);
    gc_mark_grey(t->nature);
    if (t->value) gc_mark_grey(t->value);
    if (t->additional) gc_mark_grey(t->additional);
}

static void token_sweep_free(GraceObject *self) {
    (void)self;
}


const GraceVTable lexer_token_vtable = { token_request, token_describe, token_trace, token_sweep_free };

GraceObject *lexer_token_new(int line, int column, const char *nature, GraceObject *value, GraceObject *additional) {
    LexerToken *t = (LexerToken *)gc_alloc(sizeof(LexerToken));
    t->base.vt = &lexer_token_vtable;
    t->line = grace_number_new(line);
    t->column = grace_number_new(column);
    t->nature = grace_string_new(nature);
    t->value = value;
    t->additional = additional;
    return (GraceObject *)t;
}

GraceObject *lexer_token_newg(GraceObject *line, GraceObject *column, GraceObject *nature, GraceObject *value, GraceObject *additional) {
    LexerToken *t = (LexerToken *)gc_alloc(sizeof(LexerToken));
    t->base.vt = &lexer_token_vtable;
    t->line = line;
    t->column = column;
    t->nature = nature;
    t->value = value;
    t->additional = additional;
    return (GraceObject *)t;
}

/* Lexer state: inside of memos & the lexer itself */
typedef struct LexerState {
    GraceObject base;
    const char *source; // borrowed from source_go, so not freed and source_go kept alive
    size_t length;
    GraceObject *source_go; // Null if source from a non-Grace string
    size_t index;
    int line;
    int column;
    int indentColumn;
    int lineStart;
    GraceObject *currentToken;
} LexerState;

static PendingStep *lexer_state_request(GraceObject *self, Env *env,
                                      const char *name,
                                      GraceObject **args, int nargs, Cont *k) {
    (void)env;(void)args;(void)nargs;
    if (strcmp(name, "==(0)")==0) return cont_apply(k, grace_bool(self == args[0]));
    if (strcmp(name, "asString(0)")==0) {
        return cont_apply(k, grace_string_new("lexer state"));
    }
    grace_fatal("No method '%s' on LexerMemo", name);
}

static const char *lexer_state_describe(GraceObject *self) { (void)self; return "LexerMemo"; }

static void lexer_state_trace(GraceObject *self) {
    LexerState *ls = (LexerState *)self;
    if (ls->source_go) gc_mark_grey(ls->source_go);
    if (ls->currentToken) gc_mark_grey(ls->currentToken);
}

static void lexer_state_sweep_free(GraceObject *self) {
    (void)self;
}

const GraceVTable lexer_state_vtable = { lexer_state_request, lexer_state_describe, lexer_state_trace, lexer_state_sweep_free };

GraceObject *lexer_state_new(const char *source) {
    LexerState *ls = (LexerState *)gc_alloc(sizeof(LexerState));
    ls->base.vt = &lexer_state_vtable;
    ls->source = str_dup(source);
    ls->index = 0;
    ls->line = 1;
    ls->column = 1;
    ls->indentColumn = 1;
    ls->lineStart = 0;
    ls->currentToken = NULL;
    ls->source_go = NULL;
    ls->length = strlen(source);
    return (GraceObject *)ls;
}

/* Lexer object itself: piggybacks LexerState
 * No additional data at present.
 */
typedef struct {
    LexerState state;
} LexerObject;

/* Create memo */
GraceObject *lexer_state_clone(LexerObject *lex) {
    LexerState *ls = (LexerState *)gc_alloc(sizeof(LexerState));
    ls->base.vt = &lexer_state_vtable;
    ls->source = lex->state.source;
    ls->index = lex->state.index;
    ls->line = lex->state.line;
    ls->column = lex->state.column;
    ls->indentColumn = lex->state.indentColumn;
    ls->lineStart = lex->state.lineStart;
    ls->currentToken = lex->state.currentToken;
    ls->source_go = lex->state.source_go;
    ls->length = lex->state.length;
    return (GraceObject *)ls;
}

/* Restore from memo */
void lexer_state_update(LexerObject *lex, LexerState *newState) {
    lex->state.source       = newState->source;
    lex->state.index        = newState->index;
    lex->state.line         = newState->line;
    lex->state.column       = newState->column;
    lex->state.indentColumn = newState->indentColumn;
    lex->state.lineStart    = newState->lineStart;
    lex->state.currentToken = newState->currentToken;
    lex->state.source_go    = newState->source_go;
    lex->state.length       = newState->length;
}

/* These should be aligned with lexer.grace versions */
int isIdentifierStart(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int isKeyword(const char *s) {
    return strcmp(s, "var") == 0 || strcmp(s, "def") == 0 || strcmp(s, "method") == 0 || strcmp(s, "object") == 0 || strcmp(s, "is") == 0 || strcmp(s, "return") == 0 || strcmp(s, "class") == 0 || strcmp(s, "type") == 0 || strcmp(s, "import") == 0 || strcmp(s, "self") == 0 || strcmp(s, "dialect") == 0 || strcmp(s, "interface") == 0 || strcmp(s, "inherit") == 0 || strcmp(s, "use") == 0;
}

int isDigit(char c) {
    return c >= '0' && c <= '9';
}

int isOperatorChar(char c) {
    return strchr("+-*/=:|&!><.%", c) != NULL;
}

const char *lexing_error;

static PendingStep *parseError(Env *env, int line, int column, const char *message) {
    char *fullMessage = str_fmt("%s at %s%d:%d", message, lexer_modulePrefix, line, column);
    return grace_raise(env, "ParseError", fullMessage);
}


static GraceObject *lexer_object_realLexString(LexerObject *lex) {
    size_t sourceSize = lex->state.length;
    size_t startIndex = lex->state.index;
    // First, we find the end of the string/start of the interpolation
    // Second, we process that to handle escapes and create the string token
    while (lex->state.index < sourceSize) {
        char c = lex->state.source[lex->state.index];
        if (c == '"' || c == '{') {
            break;
        }
        if (c == '\\') {
            char escapeKind = lex->state.source[lex->state.index + 1];
            if (escapeKind == 'u') {
                lex->state.index += 6;
            } else if (escapeKind == 'U') {
                lex->state.index += 8;
            } else {
                lex->state.index += 2;
            }
        } else if (c == '\n') {
            lexing_error = str_fmt("Unterminated string literal; reached newline at %d:%d", lex->state.line, lex->state.column);
            return NULL;
        } else {
            lex->state.index += 1;
        }
    }
    // If we stopped because we hit the end of the source, that's an error.
    if (lex->state.index >= sourceSize) {
        lexing_error = str_fmt("Unterminated string literal; reached end of file at %d:%d", lex->state.line, lex->state.column);
        return NULL;
    }
    // String is now from startIndex to lex->state.index-1, with escapes.
    size_t rawlen = lex->state.index - startIndex;
    char *raw = str_ndup(lex->state.source + startIndex, rawlen);
    char *processed = malloc(rawlen + 1); // processed string will be at most as long as raw
    size_t ri = 0;
    for (size_t i = 0; i < rawlen; i++) {
        char c = raw[i];
        if (c == '\\') {
            char escapeKind = raw[i + 1];
            if (escapeKind == 'n') {
                processed[ri++] = '\n';
                i += 1;
            } else if (escapeKind == 't') {
                processed[ri++] = '\t';
                i += 1;
            } else if (escapeKind == 'r') {
                processed[ri++] = '\r';
                i += 1;
            } else if (escapeKind == '\\') {
                processed[ri++] = '\\';
                i += 1;
            } else if (escapeKind == '"') {
                processed[ri++] = '"';
                i += 1;
            } else if (escapeKind == 'u' || escapeKind == 'U') {
                int codepoint = 0;
                int digits = (escapeKind == 'u') ? 4 : 6;
                for (int j = 0; j < digits; j++) {
                    char digit = raw[i + 2 + j];
                    codepoint <<= 4;
                    if (digit >= '0' && digit <= '9') {
                        codepoint |= (digit - '0');
                    } else if (digit >= 'a' && digit <= 'f') {
                        codepoint |= (digit - 'a' + 10);
                    } else if (digit >= 'A' && digit <= 'F') {
                        codepoint |= (digit - 'A' + 10);
                    } else {
                        lexing_error = str_fmt("Invalid Unicode escape in string literal at %d:%d", lex->state.line, lex->state.column);
                        free(raw);
                        free(processed);
                        return NULL;
                    }
                }
                if (codepoint <= 0x7F) {
                    processed[ri++] = (char)codepoint;
                } else if (codepoint <= 0x7FF) {
                    processed[ri++] = 0xC0 | ((codepoint >> 6) & 0x1F);
                    processed[ri++] = 0x80 | (codepoint & 0x3F);
                } else if (codepoint <= 0xFFFF) {
                    processed[ri++] = 0xE0 | ((codepoint >> 12) & 0x0F);
                    processed[ri++] = 0x80 | ((codepoint >> 6) & 0x3F);
                    processed[ri++] = 0x80 | (codepoint & 0x3F);
                } else {
                    processed[ri++] = 0xF0 | ((codepoint >> 18) & 0x07);
                    processed[ri++] = 0x80 | ((codepoint >> 12) & 0x3F);
                    processed[ri++] = 0x80 | ((codepoint >> 6) & 0x3F);
                    processed[ri++] = 0x80 | (codepoint & 0x3F);
                }
                i += 1 + digits;
            } else {
                lexing_error = str_fmt("Invalid escape sequence in string literal at %d:%d", lex->state.line, lex->state.column);
                free(raw);
                free(processed);
                return NULL;
            }
        } else {
            processed[ri++] = c;
        }
    }
    processed[ri] = 0;
    free(raw);
    if (lex->state.source[lex->state.index] == '{') {
        lex->state.index += 1; // skip opening brace
        GraceObject *ret = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_INTERPSTRING, grace_string_take(processed), NULL);
        return ret;
    }   else {
        lex->state.index += 1; // skip closing quote
        GraceObject *ret = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_STRING, grace_string_take(processed), NULL);
        return ret;
    }
}

/* Closely matches structure of lexer.grace's .nextToken, but updates currentToken
 * in-place as well as returning the value.
 * Should be kept up to date to match Grace version.
 */
static GraceObject *lexer_object_realNextToken(LexerObject *lex) {
    size_t sourceSize = lex->state.length;
    if (lex->state.index >= sourceSize) {
        lex->state.currentToken = lexer_token_new(lex->state.line, lex->state.column, "EOF", NULL, NULL);
        return lex->state.currentToken;
    }
    char c = lex->state.source[lex->state.index];
    lex->state.column = lex->state.index - lex->state.lineStart + 1;
    lex->state.index += 1;

    if (c == ' ') return lexer_object_realNextToken(lex);

    if (isIdentifierStart(c)) {
        size_t startIndex = lex->state.index - 1;
        if (lex->state.index >= sourceSize) {
            char nm[2] = { c, '\0' };
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_IDENTIFIER, grace_string_new(nm), NULL);
            return lex->state.currentToken;
        }
        c = lex->state.source[lex->state.index];
        while (isIdentifierStart(c) || ((c >= '0' && c <= '9') && lex->state.index < sourceSize)) {
            lex->state.index += 1;
            c = lex->state.source[lex->state.index];
        }
        char *nm = str_ndup(lex->state.source + startIndex, lex->state.index - startIndex);
        if (isKeyword(nm)) {
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_KEYWORD, grace_string_new(nm), NULL);
            return lex->state.currentToken;
        }
        GraceObject *ret = lexer_token_new(lex->state.line, lex->state.column, "IDENTIFIER", grace_string_new(nm), NULL);
        lex->state.currentToken = ret;
        return ret;
    }

    if (c == '(') {
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_LPAREN, NULL, NULL);
        return lex->state.currentToken;
    }
    if (c == ')') {
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_RPAREN, NULL, NULL);
        return lex->state.currentToken;
    }
    if (c == ',') {
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_COMMA, NULL, NULL);
        return lex->state.currentToken;
    }
    if (c == 13) {
        c = lex->state.source[lex->state.index];
        lex->state.index += 1;
    }
    if (c == 10) {
        lex->state.lineStart = lex->state.index;
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_NEWLINE, NULL, NULL);
        lex->state.line += 1;
        return lex->state.currentToken;
    }

    if (c == '{') {
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_LBRACE, NULL, NULL);
        return lex->state.currentToken;
    }
    if (c == '}') {
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_RBRACE, NULL, grace_number_new(lex->state.index));
        return lex->state.currentToken;
    }

    if (c == '"') {
        return lexer_object_realLexString(lex);
    }

    if (isDigit(c)) {
        size_t startIndex = lex->state.index - 1;
        while (isDigit(lex->state.source[lex->state.index]) && lex->state.index < sourceSize) {
            lex->state.index += 1;
        }
        if (lex->state.source[lex->state.index] == '.') {
            if (isDigit(lex->state.source[lex->state.index + 1])) {
                lex->state.index += 1;
                while (isDigit(lex->state.source[lex->state.index]) && lex->state.index < sourceSize) {
                    lex->state.index += 1;
                }
            }
        }
        char *numStr = str_ndup(lex->state.source + startIndex, lex->state.index - startIndex);
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_NUMBER, grace_string_take(numStr), NULL);
        return lex->state.currentToken;
    }

    if (isOperatorChar(c)) {
        size_t startIndex = lex->state.index - 1;
        while (isOperatorChar(lex->state.source[lex->state.index]) && lex->state.index < sourceSize) {
            lex->state.index += 1;
        }
        char *opStr = str_ndup(lex->state.source + startIndex, lex->state.index - startIndex);
        if (strcmp(opStr, ":=") == 0) {
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_ASSIGN, NULL, NULL);
            free(opStr);
            return lex->state.currentToken;
        }
        if (strcmp(opStr, "=") == 0) {
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_EQUALS, NULL, NULL);
            free(opStr);
            return lex->state.currentToken;
        }
        if (strcmp(opStr, "->")==0) {
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_ARROW, NULL, NULL);
            free(opStr);
            return lex->state.currentToken;
        }
        if (strcmp(opStr, ":") == 0) {
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_COLON, NULL, NULL);
            free(opStr);
            return lex->state.currentToken;
        }
        if (strcmp(opStr, ".") == 0) {
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_DOT, NULL, NULL);
            free(opStr);
            return lex->state.currentToken;
        }
        if (strlen(opStr) >= 2 && opStr[0] == '/' && opStr[1] == '/') {
            // Comment: skip to end of line
            size_t commentStart = startIndex + 2;
            while (lex->state.index < sourceSize && lex->state.source[lex->state.index] != '\n') {
                lex->state.index += 1;
            }
            char *commentText = str_ndup(lex->state.source + commentStart, lex->state.index - commentStart);
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_COMMENT, grace_string_take(commentText), NULL);
            free(opStr);
            return lex->state.currentToken;
        }
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_OPERATOR, grace_string_take(opStr), NULL);
        return lex->state.currentToken;
    }

    if (c == '[') {
        if (lex->state.source[lex->state.index] == '[') {
            lex->state.index += 1;
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_LGENERIC, NULL, NULL);
            return lex->state.currentToken;
        }
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_LSQUARE, NULL, NULL);
        return lex->state.currentToken;
    }

    if (c == ']') {
        if (lex->state.source[lex->state.index] == ']') {
            lex->state.index += 1;
            lex->state.currentToken = lexer_token_newg(
                grace_number_new(lex->state.line), grace_number_new(lex->state.column),
                TK_NATURE_RGENERIC, NULL, NULL);
            return lex->state.currentToken;
        }
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_RSQUARE, NULL, NULL);
        return lex->state.currentToken;
    }

    if (c == ';') {
        lex->state.currentToken = lexer_token_newg(
            grace_number_new(lex->state.line), grace_number_new(lex->state.column),
            TK_NATURE_SEMICOLON, NULL, NULL);
        return lex->state.currentToken;
    }

    if (c == '\t') {
        lexing_error = "Illegal tab (0+0009) character in source. Use spaces for indentation or \\u0009 inside a string.";
        return NULL;
    }

    lexing_error = str_fmt("Unexpected character '%c' in input at %d:%d", c, lex->state.line, lex->state.column);
    return NULL;
}

static PendingStep *lexer_object_nextToken(Env *env, Cont *k, LexerObject *lex) {
    GraceObject *token = lexer_object_realNextToken(lex);
    if (token)
        return cont_apply(k, token);
    return grace_raise(env, "ParseError", lexing_error);
}


static PendingStep *lexer_object_advance(Env *env, Cont *k, LexerObject *lex) {
    lex->state.currentToken = lexer_object_realNextToken(lex);
    if (!lex->state.currentToken)
        return grace_raise(env, "ParseError", lexing_error);
    if (lex->state.currentToken->vt != &lexer_token_vtable)
        grace_fatal("Internal error: lexer produced non-token object");
    if (((LexerToken *)lex->state.currentToken)->nature == TK_NATURE_NEWLINE) {
        int lineStart = lex->state.index;
        int numSpace = 0;
        size_t idx = lineStart;
        while (idx < lex->state.length && lex->state.source[idx] == ' ') {
            numSpace += 1;
            idx += 1;
        }
        if (numSpace > lex->state.indentColumn) {
            return lexer_object_advance(env, k, lex);
        }
    }
    return cont_apply(k, lex->state.currentToken);
}

static PendingStep *lexer_object_request(GraceObject *self, Env *env,
                                      const char *name,
                                      GraceObject **args, int nargs, Cont *k) {
    (void)env;(void)args;(void)nargs;(void)self;
    if (strcmp(name, "current(0)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        return cont_apply(k, lex->state.currentToken);
    }
    if (strcmp(name, "advance(0)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        return lexer_object_advance(env, k, lex);
    }
    if (strcmp(name, "startStringAt(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *pos = args[0];
        if (pos->vt != &grace_number_vtable)
            grace_fatal("startStringAt(1) requires a number argument");
        int index = grace_number_val(pos);
        lex->state.index = index;
        lex->state.currentToken = lexer_object_realLexString(lex);
        if (!lex->state.currentToken)
            return grace_raise(env, "ParseError", lexing_error);
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "indentColumn:=(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *col = args[0];
        if (col->vt != &grace_number_vtable)
            grace_fatal("indentColumn:=(1) requires a number argument");
        lex->state.indentColumn = (int)grace_number_val(col);
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "==(0)")==0) return cont_apply(k, grace_bool(self == args[0]));
    if (strcmp(name, "asString(0)")==0) {
        return cont_apply(k, grace_string_new("lexer object"));
    }
    if (strcmp(name, "save(0)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        return cont_apply(k, (GraceObject *)lexer_state_clone(lex));
    }
    if (strcmp(name, "restore(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        LexerState *state = (LexerState *)args[0];
        if (state->base.vt != &lexer_state_vtable)
            grace_fatal("restore(1) requires a lexer state argument");
        // Copy fields rather than struct because of GC data
        lexer_state_update(lex, state);
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "nextToken(0)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        return lexer_object_nextToken(env, k, lex);
    }
    if (strcmp(name, "expectToken(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *expected = args[0];
        if (expected->vt != &grace_string_vtable)
            grace_fatal("expectToken(1) requires a string argument");
        GraceObject *token = lex->state.currentToken;
        if (!token)
            return grace_raise(env, "ParseError", "Unexpected end of input");
        if (token->vt != &lexer_token_vtable)
            grace_fatal("Internal error: lexer produced non-token object");
        const char *nature = grace_string_val(((LexerToken *)token)->nature);
        if (strcmp(nature, grace_string_val(expected)) != 0) {
            lexing_error = str_fmt("Expected token of nature '%s' but got '%s'",
                grace_string_val(expected), nature);
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        return cont_apply(k, token);
    }
    if (strcmp(name, "expectToken(1)for(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *expected = args[0];
        GraceObject *purpose = args[1];
        GraceObject *token = lex->state.currentToken;
        if (!token)
            return grace_raise(env, "ParseError", "Unexpected end of input");
        if (token->vt != &lexer_token_vtable)
            grace_fatal("Internal error: lexer produced non-token object");
        const char *nature = grace_string_val(((LexerToken *)token)->nature);
        const char *purposeStr = grace_string_val(purpose);
        if (strcmp(nature, grace_string_val(expected)) != 0) {
            lexing_error = str_fmt("Expected token of nature '%s' as %s but got '%s'",
                grace_string_val(expected), purposeStr, nature);
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        return cont_apply(k, token);
    }
    if (strcmp(name, "skipWhitespace(0)") == 0) {
        // Actually only skip newlines
        LexerObject *lex = (LexerObject *)self;
        while (lex->state.currentToken && lex->state.currentToken->vt == &lexer_token_vtable &&
               ((LexerToken*)lex->state.currentToken)->nature == TK_NATURE_NEWLINE) {
            lex->state.currentToken = lexer_object_realNextToken(lex);
        }
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "expectSymbol(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *expected = args[0];
        if (expected->vt != &grace_string_vtable)
            grace_fatal("expectSymbol(1) requires a string argument");
        GraceObject *token = lex->state.currentToken;
        if (!token)
            return grace_raise(env, "ParseError", "Unexpected end of input");
        if (token->vt != &lexer_token_vtable)
            grace_fatal("Internal error: lexer produced non-token object");
        const char *nature = grace_string_val(((LexerToken *)token)->nature);
        if (strcmp(nature, grace_string_val(expected)) != 0) {
            lexing_error = str_fmt("Expected '%s' but got '%s'",
                grace_string_val(expected), nature);
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "expectSymbol(1)explaining(1)") == 0) {
        // ?? Real lexer seems to handle this identically to expectSymbol
        LexerObject *lex = (LexerObject *)self;
        GraceObject *expected = args[0];
        GraceObject *msg = args[1];
        if (expected->vt != &grace_string_vtable)
            grace_fatal("expectSymbol(1) requires a string argument");
        GraceObject *token = lex->state.currentToken;
        if (!token)
            return grace_raise(env, "ParseError", "Unexpected end of input");
        if (token->vt != &lexer_token_vtable)
            grace_fatal("Internal error: lexer produced non-token object");
        const char *nature = grace_string_val(((LexerToken *)token)->nature);
        if (strcmp(nature, grace_string_val(expected)) != 0) {
            lexing_error = str_fmt("Expected '%s' but got '%s'; %s",
                grace_string_val(expected), nature, grace_string_val(msg));
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "expectSymbol(1)or(1)explaining(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *expected = args[0];
        GraceObject *alternative = args[1];
        GraceObject *msg = args[2];
        if (expected->vt != &grace_string_vtable)
            grace_fatal("expectSymbol(1)or(1)explaining(1) requires a string argument in position 1");
        if (alternative->vt != &grace_string_vtable)
            grace_fatal("expectSymbol(1)or(1)explaining(1) requires a string argument in position 2");
        if (msg->vt != &grace_string_vtable)
            grace_fatal("expectSymbol(1)or(1)explaining(1) requires a string argument in position 3");
        GraceObject *token = lex->state.currentToken;
        if (!token)
            return grace_raise(env, "ParseError", "Unexpected end of input");
        if (token->vt != &lexer_token_vtable)
            grace_fatal("Internal error: lexer produced non-token object");
        const char *nature = grace_string_val(((LexerToken *)token)->nature);
        if (strcmp(nature, grace_string_val(expected)) != 0 && strcmp(nature, grace_string_val(alternative)) != 0) {
            lexing_error = str_fmt("Expected '%s' or '%s' but got '%s'; %s",
                grace_string_val(expected), grace_string_val(alternative), nature, grace_string_val(msg));
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        return cont_apply(k, grace_done);
    }
    if (strcmp(name, "expectKeyword(1)") == 0) {
        LexerObject *lex = (LexerObject *)self;
        GraceObject *expected = args[0];
        if (expected->vt != &grace_string_vtable)
            grace_fatal("expectKeyword(1) requires a string argument");
        LexerToken *token = (LexerToken *)lex->state.currentToken;
        if (!token)
            return grace_raise(env, "ParseError", "Unexpected end of input");
        if (token->base.vt != &lexer_token_vtable)
            grace_fatal("Internal error: lexer produced non-token object");
        const char *nature = grace_string_val(token->nature);
        if (token->nature != TK_NATURE_KEYWORD) {
            lexing_error = str_fmt("Expected keyword '%s' but got '%s'",
                grace_string_val(expected), nature);
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        const char *value = grace_string_val(token->value);
        if (strcmp(value, grace_string_val(expected)) != 0) {
            lexing_error = str_fmt("Expected keyword '%s' but got '%s'",
                grace_string_val(expected), value);
            return parseError(env, lex->state.line, lex->state.column, lexing_error);
        }
        return cont_apply(k, grace_done);
    }
    grace_fatal("No method '%s' on LexerObject", name);
}


static const char *lexer_object_describe(GraceObject *self) { (void)self; return "LexerObject"; }

const GraceVTable lexer_object_vtable = { lexer_object_request, lexer_object_describe, lexer_state_trace, lexer_state_sweep_free };


GraceObject *lexer_new(const char *source) {
    LexerObject *lex = (LexerObject *)gc_alloc(sizeof(LexerObject));
    lex->state.base.vt = &lexer_object_vtable;
    lex->state.source = str_dup(source);
    lex->state.index = 0;
    lex->state.line = 1;
    lex->state.column = 1;
    lex->state.lineStart = 0;
    lex->state.currentToken = NULL;
    lex->state.source_go = NULL;
    lex->state.length = strlen(source);
    return (GraceObject *)lex;
}

GraceObject *lexer_newg(GraceObject *source_go) {
    LexerObject *lex = (LexerObject *)gc_alloc(sizeof(LexerObject));
    lex->state.base.vt = &lexer_object_vtable;
    lex->state.source = grace_string_val(source_go);
    lex->state.index = 0;
    lex->state.line = 1;
    lex->state.column = 1;
    lex->state.lineStart = 0;
    lex->state.currentToken = NULL;
    lex->state.source_go = source_go;
    lex->state.length = strlen(lex->state.source);
    return (GraceObject *)lex;
}

/* Lexer *module* object */

LexerObject *last_lexer;
int init_indentColumn = 0;

static PendingStep *lexer_indentColumn_assn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;(void)data;(void)env;
    LexerObject *lex = last_lexer;
    GraceObject *col = args[0];
    if (col->vt != &grace_number_vtable)
        grace_fatal("indentColumn:=(1) requires a number argument");
    if (lex)
        lex->state.indentColumn = (int)grace_number_val(col);
    else
        init_indentColumn = (int)grace_number_val(col);
    return cont_apply(k, grace_done);
}

static PendingStep *lexer_indentColumn_fn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;(void)data;(void)env;
    LexerObject *lex = last_lexer;
    return cont_apply(k, grace_number_new(lex->state.indentColumn));
}

static PendingStep *lexer_modulePrefix_assn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;(void)data;(void)env;
    lexer_modulePrefix = str_dup(grace_string_val(args[0]));
    return cont_apply(k, grace_done);
}

static PendingStep *lexer_modulePrefix_fn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;(void)data;(void)env;
    return cont_apply(k, grace_string_new(lexer_modulePrefix));
}

static PendingStep *lexer_digitToNumber_fn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;(void)data;(void)env;
    const char *s = grace_string_val(args[1]);
    char c = s[0];
    if (c >= '0' && c <= '9') {
        return cont_apply(k, grace_number_new(c - '0'));
    }
    GraceObject *tokObj = args[0];
    LexerToken *token = (LexerToken *)tokObj;
    lexing_error = str_fmt("Unexpected digit: '%c'", c);
    return parseError(env, grace_number_val(token->line), grace_number_val(token->column), lexing_error);
}

static PendingStep *lexer_new_fn(GraceObject *self, Env *env, GraceObject **args,
                                   int nargs, Cont *k, void *data) {
    (void)self;(void)args;(void)nargs;(void)data;(void)env;
    GraceObject *obj = lexer_newg(args[0]);
    LexerObject *lex = (LexerObject *)obj;
    last_lexer = lex;
    lex->state.currentToken = lexer_object_realNextToken(lex);
    if (!lex->state.currentToken)
        return grace_raise(env, "ParseError", lexing_error);
    return cont_apply(k, obj);
}

GraceObject *make_lexer_module() {
    GraceObject *mod = grace_user_new(NULL);
    user_add_method(mod, "indentColumn:=(1)", lexer_indentColumn_assn, NULL);
    user_add_method(mod, "indentColumn(0)", lexer_indentColumn_fn, NULL);
    user_add_method(mod, "lexer(1)", lexer_new_fn, NULL);
    user_add_method(mod, "modulePrefix:=(1)", lexer_modulePrefix_assn, NULL);
    user_add_method(mod, "modulePrefix(0)", lexer_modulePrefix_fn, NULL);
    user_add_method(mod, "digitToNumber(2)", lexer_digitToNumber_fn, NULL);
    if (!TK_NATURE_EOF) {
        TK_NATURE_EOF = grace_string_new("EOF");
        TK_NATURE_EOF->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_NUMBER) {
        TK_NATURE_NUMBER = grace_string_new("NUMBER");
        TK_NATURE_NUMBER->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_LPAREN) {
        TK_NATURE_LPAREN = grace_string_new("LPAREN");
        TK_NATURE_LPAREN->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_RPAREN) {
        TK_NATURE_RPAREN = grace_string_new("RPAREN");
        TK_NATURE_RPAREN->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_LGENERIC) {
        TK_NATURE_LGENERIC = grace_string_new("LGENERIC");
        TK_NATURE_LGENERIC->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_RGENERIC) {
        TK_NATURE_RGENERIC = grace_string_new("RGENERIC");
        TK_NATURE_RGENERIC->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_LBRACE) {
        TK_NATURE_LBRACE = grace_string_new("LBRACE");
        TK_NATURE_LBRACE->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_RBRACE) {
        TK_NATURE_RBRACE = grace_string_new("RBRACE");
        TK_NATURE_RBRACE->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_LSQUARE) {
        TK_NATURE_LSQUARE = grace_string_new("LSQUARE");
        TK_NATURE_LSQUARE->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_RSQUARE) {
        TK_NATURE_RSQUARE = grace_string_new("RSQUARE");
        TK_NATURE_RSQUARE->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_COMMA) {
        TK_NATURE_COMMA = grace_string_new("COMMA");
        TK_NATURE_COMMA->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_DOT) {
        TK_NATURE_DOT = grace_string_new("DOT");
        TK_NATURE_DOT->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_IDENTIFIER) {
        TK_NATURE_IDENTIFIER = grace_string_new("IDENTIFIER");
        TK_NATURE_IDENTIFIER->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_KEYWORD) {
        TK_NATURE_KEYWORD = grace_string_new("KEYWORD");
        TK_NATURE_KEYWORD->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_OPERATOR) {
        TK_NATURE_OPERATOR = grace_string_new("OPERATOR");
        TK_NATURE_OPERATOR->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_STRING) {
        TK_NATURE_STRING = grace_string_new("STRING");
        TK_NATURE_STRING->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_INTERPSTRING) {
        TK_NATURE_INTERPSTRING = grace_string_new("INTERPSTRING");
        TK_NATURE_INTERPSTRING->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_EQUALS) {
        TK_NATURE_EQUALS = grace_string_new("EQUALS");
        TK_NATURE_EQUALS->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_ASSIGN) {
        TK_NATURE_ASSIGN = grace_string_new("ASSIGN");
        TK_NATURE_ASSIGN->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_ARROW) {
        TK_NATURE_ARROW = grace_string_new("ARROW");
        TK_NATURE_ARROW->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_NEWLINE) {
        TK_NATURE_NEWLINE = grace_string_new("NEWLINE");
        TK_NATURE_NEWLINE->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_COLON) {
        TK_NATURE_COLON = grace_string_new("COLON");
        TK_NATURE_COLON->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_SEMICOLON) {
        TK_NATURE_SEMICOLON = grace_string_new("SEMICOLON");
        TK_NATURE_SEMICOLON->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_COMMENT) {
        TK_NATURE_COMMENT = grace_string_new("COMMENT");
        TK_NATURE_COMMENT->gc_color = GC_STATIC;
    }
    if (!TK_NATURE_ERROR) {
        TK_NATURE_ERROR = grace_string_new("ERROR");
        TK_NATURE_ERROR->gc_color = GC_STATIC;
    }
    return mod;
}