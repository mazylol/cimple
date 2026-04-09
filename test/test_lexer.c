#include "../include/dynamic.h"
#include "../include/lexer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

struct Token *get_token_at(struct dynamic *tokens, size_t index) {
    return (struct Token *)tokens->elements[index];
}

void test_basic_variable_declaration() {
    const char *source = "var x = 42;";
    FILE *fp = tmpfile();
    fputs(source, fp);
    rewind(fp);

    struct dynamic tokens = get_tokens(fp);

    assert(tokens.size == 6);

    struct Token *t;

    t = get_token_at(&tokens, 0);
    assert(t->kind == TOK_KW_VAR);
    assert(strcmp(t->lexeme, "var") == 0);

    t = get_token_at(&tokens, 1);
    assert(t->kind == TOK_IDENT);
    assert(strcmp(t->lexeme, "x") == 0);

    t = get_token_at(&tokens, 2);
    assert(t->kind == TOK_EQUAL);
    assert(strcmp(t->lexeme, "=") == 0);

    t = get_token_at(&tokens, 3);
    assert(t->kind == TOK_NUMBER);
    assert(strcmp(t->lexeme, "42") == 0);

    t = get_token_at(&tokens, 4);
    assert(t->kind == TOK_SEMICOLON);
    assert(strcmp(t->lexeme, ";") == 0);

    t = get_token_at(&tokens, 5);
    assert(t->kind == TOK_EOF);

    fclose(fp);

    dynamic_free(&tokens);
}
