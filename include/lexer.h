#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

enum TokenKind {
    TOK_EOF = 0,
    TOK_KW_VAR,
    TOK_KW_FUN,
    TOK_KW_IF,
    TOK_KW_WHILE,
    TOK_KW_FOR,
    TOK_KW_STRUCT,
    TOK_TYPE_NUMBER,
    TOK_TYPE_STRING,
    TOK_TYPE_VOID,
    TOK_TYPE_INT,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_CHAR,
    TOK_COLON,
    TOK_SEMICOLON,
    TOK_EQUAL,
    TOK_PLUS,
    TOK_PLUS_EQUAL,
    TOK_PLUS_PLUS,
    TOK_LT,
    TOK_GT,
    TOK_LE,
    TOK_GE,
    TOK_EQEQ,
    TOK_BANG_EQ,
    TOK_LBRACE,
    TOK_RBRACE,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_COMMA,
    TOK_UNKNOWN
};

struct Token {
    enum TokenKind kind;
    char lexeme[128];
};

void print_token_legend(void);
void token_debug_print(struct dynamic *tokens);
struct dynamic get_tokens(FILE *fp);

#endif // LEXER_H
