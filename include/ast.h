#ifndef AST_H
#define AST_H

#include "dynamic.h"
#include "lexer.h"

struct Tree {
    struct Token token;

    struct Tree **nodes;
};

struct Tree build_tree(struct dynamic *tokens);

#endif // AST_H
