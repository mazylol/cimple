#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "../include/dynamic.h"
#include "../include/lexer.h"

static int is_ident_start(int c) {
    return isalpha(c) || c == '_';
}

static int is_ident_continue(int c) {
    return isalnum(c) || c == '_';
}

static enum TokenKind keyword_kind(const char *text) {
    if (strcmp(text, "var") == 0) {
        return TOK_KW_VAR;
    }
    if (strcmp(text, "fun") == 0) {
        return TOK_KW_FUN;
    }
    if (strcmp(text, "if") == 0) {
        return TOK_KW_IF;
    }
    if (strcmp(text, "while") == 0) {
        return TOK_KW_WHILE;
    }
    if (strcmp(text, "for") == 0) {
        return TOK_KW_FOR;
    }
    if (strcmp(text, "struct") == 0) {
        return TOK_KW_STRUCT;
    }
    if (strcmp(text, "number") == 0) {
        return TOK_TYPE_NUMBER;
    }
    if (strcmp(text, "string") == 0) {
        return TOK_TYPE_STRING;
    }
    if (strcmp(text, "void") == 0) {
        return TOK_TYPE_VOID;
    }
    if (strcmp(text, "int") == 0) {
        return TOK_TYPE_INT;
    }
    return TOK_IDENT;
}

const char *token_enum_name(enum TokenKind kind) {
    switch (kind) {
    case TOK_EOF:
        return "TOK_EOF";
    case TOK_KW_VAR:
        return "TOK_KW_VAR";
    case TOK_KW_FUN:
        return "TOK_KW_FUN";
    case TOK_KW_IF:
        return "TOK_KW_IF";
    case TOK_KW_WHILE:
        return "TOK_KW_WHILE";
    case TOK_KW_FOR:
        return "TOK_KW_FOR";
    case TOK_KW_STRUCT:
        return "TOK_KW_STRUCT";
    case TOK_TYPE_NUMBER:
        return "TOK_TYPE_NUMBER";
    case TOK_TYPE_STRING:
        return "TOK_TYPE_STRING";
    case TOK_TYPE_VOID:
        return "TOK_TYPE_VOID";
    case TOK_TYPE_INT:
        return "TOK_TYPE_INT";
    case TOK_IDENT:
        return "TOK_IDENT";
    case TOK_NUMBER:
        return "TOK_NUMBER";
    case TOK_STRING:
        return "TOK_STRING";
    case TOK_CHAR:
        return "TOK_CHAR";
    case TOK_COLON:
        return "TOK_COLON";
    case TOK_SEMICOLON:
        return "TOK_SEMICOLON";
    case TOK_EQUAL:
        return "TOK_EQUAL";
    case TOK_PLUS:
        return "TOK_PLUS";
    case TOK_PLUS_EQUAL:
        return "TOK_PLUS_EQUAL";
    case TOK_PLUS_PLUS:
        return "TOK_PLUS_PLUS";
    case TOK_LT:
        return "TOK_LT";
    case TOK_GT:
        return "TOK_GT";
    case TOK_LE:
        return "TOK_LE";
    case TOK_GE:
        return "TOK_GE";
    case TOK_EQEQ:
        return "TOK_EQEQ";
    case TOK_BANG_EQ:
        return "TOK_BANG_EQ";
    case TOK_LBRACE:
        return "TOK_LBRACE";
    case TOK_RBRACE:
        return "TOK_RBRACE";
    case TOK_LPAREN:
        return "TOK_LPAREN";
    case TOK_RPAREN:
        return "TOK_RPAREN";
    case TOK_COMMA:
        return "TOK_COMMA";
    default:
        return "TOK_UNKNOWN";
    }
}

const char *token_debug_mapping(enum TokenKind kind) {
    switch (kind) {
    case TOK_EOF:
        return "EOF";
    case TOK_KW_VAR:
        return "var";
    case TOK_KW_FUN:
        return "fun";
    case TOK_KW_IF:
        return "if";
    case TOK_KW_WHILE:
        return "while";
    case TOK_KW_FOR:
        return "for";
    case TOK_KW_STRUCT:
        return "struct";
    case TOK_TYPE_NUMBER:
        return "number";
    case TOK_TYPE_STRING:
        return "string";
    case TOK_TYPE_VOID:
        return "void";
    case TOK_TYPE_INT:
        return "int";
    case TOK_IDENT:
        return "<identifier>";
    case TOK_NUMBER:
        return "<number>";
    case TOK_STRING:
        return "<string>";
    case TOK_CHAR:
        return "<char>";
    case TOK_COLON:
        return ":";
    case TOK_SEMICOLON:
        return ";";
    case TOK_EQUAL:
        return "=";
    case TOK_PLUS:
        return "+";
    case TOK_PLUS_EQUAL:
        return "+=";
    case TOK_PLUS_PLUS:
        return "++";
    case TOK_LT:
        return "<";
    case TOK_GT:
        return ">";
    case TOK_LE:
        return "<=";
    case TOK_GE:
        return ">=";
    case TOK_EQEQ:
        return "==";
    case TOK_BANG_EQ:
        return "!=";
    case TOK_LBRACE:
        return "{";
    case TOK_RBRACE:
        return "}";
    case TOK_LPAREN:
        return "(";
    case TOK_RPAREN:
        return ")";
    case TOK_COMMA:
        return ",";
    default:
        return "<unknown>";
    }
}

void print_token_legend(void) {
    for (int i = TOK_EOF; i <= TOK_UNKNOWN; i++) {
        enum TokenKind kind = (enum TokenKind)i;
        printf("%-16s -> %s\n", token_enum_name(kind), token_debug_mapping(kind));
    }
}

static struct Token make_simple(enum TokenKind kind, const char *lexeme) {
    struct Token tok;
    tok.kind = kind;
    tok.lexeme[0] = '\0';
    if (lexeme != NULL) {
        strncpy(tok.lexeme, lexeme, sizeof(tok.lexeme) - 1);
        tok.lexeme[sizeof(tok.lexeme) - 1] = '\0';
    }
    return tok;
}

struct Token _next_token(FILE *fp) {
    int c;
    char buf[128];
    int i = 0;

    while (1) {
        c = fgetc(fp);
        while (c != EOF && isspace(c)) {
            c = fgetc(fp);
        }

        if (c == '/') {
            int next = fgetc(fp);
            if (next == '*') {
                int prev = 0;
                while ((c = fgetc(fp)) != EOF) {
                    if (prev == '*' && c == '/') {
                        break;
                    }
                    prev = c;
                }
                continue;
            }
            if (next == '/') {
                while ((c = fgetc(fp)) != EOF && c != '\n') {
                }
                continue;
            }
            if (next != EOF) {
                ungetc(next, fp);
            }
            buf[0] = '/';
            buf[1] = '\0';
            return make_simple(TOK_UNKNOWN, buf);
        }
        break;
    }

    if (c == EOF) {
        return make_simple(TOK_EOF, "EOF");
    }

    if (is_ident_start(c)) {
        buf[i++] = (char)c;
        while ((c = fgetc(fp)) != EOF && is_ident_continue(c)) {
            if (i < (int)sizeof(buf) - 1) {
                buf[i++] = (char)c;
            }
        }
        buf[i] = '\0';
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(keyword_kind(buf), buf);
    }

    if (isdigit(c)) {
        buf[i++] = (char)c;
        while ((c = fgetc(fp)) != EOF && isdigit(c)) {
            if (i < (int)sizeof(buf) - 1) {
                buf[i++] = (char)c;
            }
        }
        buf[i] = '\0';
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(TOK_NUMBER, buf);
    }

    if (c == '"') {
        while ((c = fgetc(fp)) != EOF && c != '"') {
            if (c == '\\') {
                int escaped = fgetc(fp);
                if (escaped == EOF) {
                    break;
                }
                if (i < (int)sizeof(buf) - 1) {
                    buf[i++] = (char)c;
                }
                c = escaped;
            }
            if (i < (int)sizeof(buf) - 1) {
                buf[i++] = (char)c;
            }
        }
        buf[i] = '\0';
        return make_simple(TOK_STRING, buf);
    }

    if (c == '\'') {
        c = fgetc(fp);
        if (c == '\\') {
            if (i < (int)sizeof(buf) - 1) {
                buf[i++] = (char)c;
            }
            c = fgetc(fp);
        }
        if (c != EOF && c != '\'') {
            if (i < (int)sizeof(buf) - 1) {
                buf[i++] = (char)c;
            }
            c = fgetc(fp);
        }
        if (c != '\'') {
            while (c != EOF && c != '\'') {
                c = fgetc(fp);
            }
        }
        buf[i] = '\0';
        return make_simple(TOK_CHAR, buf);
    }

    switch (c) {
    case ':':
        return make_simple(TOK_COLON, ":");
    case ';':
        return make_simple(TOK_SEMICOLON, ";");
    case '=':
        c = fgetc(fp);
        if (c == '=') {
            return make_simple(TOK_EQEQ, "==");
        }
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(TOK_EQUAL, "=");
    case '+':
        c = fgetc(fp);
        if (c == '=') {
            return make_simple(TOK_PLUS_EQUAL, "+=");
        }
        if (c == '+') {
            return make_simple(TOK_PLUS_PLUS, "++");
        }
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(TOK_PLUS, "+");
    case '<':
        c = fgetc(fp);
        if (c == '=') {
            return make_simple(TOK_LE, "<=");
        }
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(TOK_LT, "<");
    case '>':
        c = fgetc(fp);
        if (c == '=') {
            return make_simple(TOK_GE, ">=");
        }
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(TOK_GT, ">");
    case '!':
        c = fgetc(fp);
        if (c == '=') {
            return make_simple(TOK_BANG_EQ, "!=");
        }
        if (c != EOF) {
            ungetc(c, fp);
        }
        return make_simple(TOK_UNKNOWN, "!");
    case '{':
        return make_simple(TOK_LBRACE, "{");
    case '}':
        return make_simple(TOK_RBRACE, "}");
    case '(':
        return make_simple(TOK_LPAREN, "(");
    case ')':
        return make_simple(TOK_RPAREN, ")");
    case ',':
        return make_simple(TOK_COMMA, ",");
    default:
        buf[0] = (char)c;
        buf[1] = '\0';
        return make_simple(TOK_UNKNOWN, buf);
    }
}

struct dynamic get_tokens(FILE *fp) {
    struct dynamic tokens = dynamic_init(sizeof(struct Token));

    while (1) {
        struct Token tok = _next_token(fp);
        dynamic_push(&tokens, &tok);

        if (tok.kind == TOK_EOF) {
            break;
        }
    }

    return tokens;
}

void token_debug_print(struct dynamic *tokens) {
    for (size_t i = 0; i < tokens->size; i++) {
        struct Token *tok = (struct Token *)tokens->elements[i];
        printf("%-16s -> %s", token_enum_name(tok->kind), token_debug_mapping(tok->kind));
        if (tok->lexeme[0] != '\0') {
            printf(" (lexeme: %s)", tok->lexeme);
        }
        printf("\n");
    }
}
