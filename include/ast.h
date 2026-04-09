#ifndef AST_H
#define AST_H

#include <stddef.h>

#include "dynamic.h"
#include "lexer.h"

struct Tree {
    struct Token *token;
    size_t branch_count;

    struct Tree **nodes;
};

struct Tree init_tree(size_t branch_count, const struct Token *token);
struct Tree init_tree_one_branch(const struct Token *token);
struct Tree init_tree_two_branches(const struct Token *token);
struct Tree *tree_add_node(struct Tree *tree, size_t branch_index, const struct Token *token);
struct Tree *tree_add_node_with_branches(struct Tree *tree, size_t branch_index, size_t node_branch_count,
                                         const struct Token *token);
struct Tree *tree_get_node(struct Tree *tree, size_t branch_index, size_t depth);
void tree_debug_print(const struct Tree *tree);
void tree_free(struct Tree *tree);

struct Tree build_tree(struct dynamic *tokens);

#endif // AST_H
