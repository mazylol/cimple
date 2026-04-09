#include "../include/ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct Token *clone_token(const struct Token *token) {
    if (token == NULL) {
        return NULL;
    }

    struct Token *copy = malloc(sizeof(struct Token));
    if (copy == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    memcpy(copy, token, sizeof(struct Token));
    return copy;
}

static struct Token *token_at(const struct dynamic *tokens, const size_t index) {
    return (struct Token *)tokens->elements[index];
}

struct Tree init_tree(const size_t branch_count, const struct Token *token) {
    struct Tree tree = {};
    tree.branch_count = branch_count;

    if (branch_count > 0) {
        tree.nodes = calloc(branch_count, sizeof(struct Tree *));
        if (tree.nodes == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(1);
        }
    }

    tree.token = clone_token(token);

    return tree;
}

struct Tree init_tree_one_branch(const struct Token *token) {
    return init_tree(1, token);
}

struct Tree init_tree_two_branches(const struct Token *token) {
    return init_tree(2, token);
}

struct Tree *tree_add_node(struct Tree *tree, const size_t branch_index, const struct Token *token) {
    if (tree == NULL || branch_index >= tree->branch_count) {
        return NULL;
    }
    return tree_add_node_with_branches(tree, branch_index, tree->branch_count, token);
}

struct Tree *tree_add_node_with_branches(struct Tree *tree, const size_t branch_index, const size_t node_branch_count,
                                         const struct Token *token) {
    if (tree == NULL || branch_index >= tree->branch_count) {
        return NULL;
    }

    struct Tree *cursor = tree;

    while (cursor->nodes[branch_index] != NULL) {
        cursor = cursor->nodes[branch_index];
    }

    struct Tree *new_node = malloc(sizeof(struct Tree));
    if (new_node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    *new_node = init_tree(node_branch_count, token);
    cursor->nodes[branch_index] = new_node;

    return new_node;
}

struct Tree *tree_get_node(struct Tree *tree, const size_t branch_index, const size_t depth) {
    if (tree == NULL || branch_index >= tree->branch_count) {
        return NULL;
    }

    struct Tree *cursor = tree;

    for (size_t i = 0; i < depth; i++) {
        cursor = cursor->nodes[branch_index];
        if (cursor == NULL) {
            return NULL;
        }
    }

    return cursor;
}

static void tree_print_node(const struct Tree *node, const size_t depth, const int branch_index) {
    for (size_t i = 0; i < depth; i++) {
        printf("  ");
    }

    if (branch_index >= 0) {
        printf("[branch %d] ", branch_index);
    } else {
        printf("[root] ");
    }

    if (node == NULL) {
        printf("<null>\n");
        return;
    }

    if (node->token == NULL) {
        printf("token=<null>\n");
    } else {
        printf("kind=%d lexeme=\"%s\"\n", node->token->kind, node->token->lexeme);
    }

    for (size_t i = 0; i < node->branch_count; i++) {
        tree_print_node(node->nodes[i], depth + 1, (int)i);
    }
}

void tree_debug_print(const struct Tree *tree) {
    tree_print_node(tree, 0, -1);
}

void tree_free(struct Tree *tree) {
    if (tree == NULL) {
        return;
    }

    for (size_t i = 0; i < tree->branch_count; i++) {
        tree_free(tree->nodes[i]);
        tree->nodes[i] = NULL;
    }

    free(tree->nodes);
    tree->nodes = NULL;

    free(tree->token);
    tree->token = NULL;

    tree->branch_count = 0;
}

struct AstParser {
    struct dynamic *tokens;
    size_t pos;
};

static int parser_is_at_end(const struct AstParser *parser) {
    if (parser->tokens == NULL || parser->pos >= parser->tokens->size) {
        return 1;
    }
    return token_at(parser->tokens, parser->pos)->kind == TOK_EOF;
}

static struct Token *parser_peek(const struct AstParser *parser) {
    if (parser->tokens == NULL || parser->pos >= parser->tokens->size) {
        return NULL;
    }
    return token_at(parser->tokens, parser->pos);
}

static struct Token *parser_advance(struct AstParser *parser) {
    if (!parser_is_at_end(parser)) {
        parser->pos++;
    }
    if (parser->pos == 0 || parser->tokens == NULL || parser->pos - 1 >= parser->tokens->size) {
        return NULL;
    }
    return token_at(parser->tokens, parser->pos - 1);
}

static struct Token *parser_expect(struct AstParser *parser, enum TokenKind kind, const char *message) {
    struct Token *tok = parser_peek(parser);
    if (tok == NULL || tok->kind != kind) {
        fprintf(stderr, "AST parse error: %s\n", message);
        return NULL;
    }
    return parser_advance(parser);
}

static struct Tree *tree_new_node(const size_t branch_count, const struct Token *token) {
    struct Tree *node = malloc(sizeof(struct Tree));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    *node = init_tree(branch_count, token);
    return node;
}

static struct Tree *build_expression_from_range(struct dynamic *tokens, const size_t start, const size_t end_exclusive) {
    if (tokens == NULL || start >= end_exclusive || end_exclusive > tokens->size) {
        return tree_new_node(0, NULL);
    }

    struct Tree *expr = tree_new_node(1, token_at(tokens, start));
    for (size_t i = start + 1; i < end_exclusive; i++) {
        tree_add_node(expr, 0, token_at(tokens, i));
    }
    return expr;
}

static int token_is_else(const struct Token *token) {
    return token != NULL && token->kind == TOK_IDENT && strcmp(token->lexeme, "else") == 0;
}

static struct Tree *parse_statement(struct AstParser *parser);

static struct Tree *parse_block_body(struct AstParser *parser, const struct Token *lbrace_token) {
    struct Tree **statements = NULL;
    size_t count = 0;
    size_t capacity = 0;

    while (!parser_is_at_end(parser)) {
        struct Token *tok = parser_peek(parser);
        if (tok != NULL && tok->kind == TOK_RBRACE) {
            parser_advance(parser);
            break;
        }

        struct Tree *statement = parse_statement(parser);
        if (statement == NULL) {
            continue;
        }

        if (count == capacity) {
            size_t new_capacity = capacity == 0 ? 4 : capacity * 2;
            struct Tree **new_statements = realloc(statements, sizeof(struct Tree *) * new_capacity);
            if (new_statements == NULL) {
                fprintf(stderr, "Memory allocation failed\n");
                exit(1);
            }
            statements = new_statements;
            capacity = new_capacity;
        }
        statements[count++] = statement;
    }

    struct Tree *block = tree_new_node(count, lbrace_token);
    for (size_t i = 0; i < count; i++) {
        block->nodes[i] = statements[i];
    }
    free(statements);
    return block;
}

static struct Tree *parse_parenthesized_expression(struct AstParser *parser) {
    if (parser_expect(parser, TOK_LPAREN, "Expected '(' to start expression") == NULL) {
        return tree_new_node(0, NULL);
    }

    const size_t start = parser->pos;
    size_t depth = 1;
    while (!parser_is_at_end(parser) && depth > 0) {
        struct Token *tok = parser_advance(parser);
        if (tok == NULL) {
            break;
        }
        if (tok->kind == TOK_LPAREN) {
            depth++;
        } else if (tok->kind == TOK_RPAREN) {
            depth--;
        }
    }

    if (depth != 0 || parser->pos == 0) {
        fprintf(stderr, "AST parse error: Unterminated parenthesized expression\n");
        return tree_new_node(0, NULL);
    }

    const size_t end_exclusive = parser->pos - 1;
    return build_expression_from_range(parser->tokens, start, end_exclusive);
}

static struct Tree *parse_expression_until(struct AstParser *parser, enum TokenKind delimiter) {
    const size_t start = parser->pos;
    size_t paren_depth = 0;

    while (!parser_is_at_end(parser)) {
        struct Token *tok = parser_peek(parser);
        if (tok == NULL) {
            break;
        }

        if (tok->kind == TOK_LPAREN) {
            paren_depth++;
            parser_advance(parser);
            continue;
        }
        if (tok->kind == TOK_RPAREN && paren_depth > 0) {
            paren_depth--;
            parser_advance(parser);
            continue;
        }
        if (paren_depth == 0 && tok->kind == delimiter) {
            break;
        }
        if (paren_depth == 0 && delimiter == TOK_SEMICOLON && tok->kind == TOK_RBRACE) {
            break;
        }

        parser_advance(parser);
    }

    return build_expression_from_range(parser->tokens, start, parser->pos);
}

static struct Tree *parse_var_statement(struct AstParser *parser) {
    const size_t start = parser->pos;
    parser_expect(parser, TOK_KW_VAR, "Expected 'var'");
    while (!parser_is_at_end(parser)) {
        struct Token *tok = parser_peek(parser);
        if (tok == NULL || tok->kind == TOK_SEMICOLON || tok->kind == TOK_RBRACE) {
            break;
        }
        parser_advance(parser);
    }
    struct Tree *var_node = build_expression_from_range(parser->tokens, start, parser->pos);
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after variable declaration");
    return var_node;
}

static struct Tree *parse_if_statement(struct AstParser *parser) {
    struct Token *if_token = parser_expect(parser, TOK_KW_IF, "Expected 'if'");
    struct Tree *if_node = tree_new_node(3, if_token);

    if_node->nodes[0] = parse_parenthesized_expression(parser);
    if_node->nodes[1] = parse_statement(parser);

    struct Token *maybe_else = parser_peek(parser);
    if (token_is_else(maybe_else)) {
        parser_advance(parser);
        if_node->nodes[2] = parse_statement(parser);
    }

    return if_node;
}

static struct Tree *parse_while_statement(struct AstParser *parser) {
    struct Token *while_token = parser_expect(parser, TOK_KW_WHILE, "Expected 'while'");
    struct Tree *while_node = tree_new_node(2, while_token);

    while_node->nodes[0] = parse_parenthesized_expression(parser);
    while_node->nodes[1] = parse_statement(parser);

    return while_node;
}

static struct Tree *parse_for_statement(struct AstParser *parser) {
    struct Token *for_token = parser_expect(parser, TOK_KW_FOR, "Expected 'for'");
    struct Tree *for_node = tree_new_node(4, for_token);

    if (parser_expect(parser, TOK_LPAREN, "Expected '(' after 'for'") == NULL) {
        return for_node;
    }

    for_node->nodes[0] = parse_expression_until(parser, TOK_SEMICOLON);
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after for-init");

    for_node->nodes[1] = parse_expression_until(parser, TOK_SEMICOLON);
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after for-condition");

    for_node->nodes[2] = parse_expression_until(parser, TOK_RPAREN);
    parser_expect(parser, TOK_RPAREN, "Expected ')' after for-step");

    for_node->nodes[3] = parse_statement(parser);
    return for_node;
}

static struct Tree *parse_return_statement(struct AstParser *parser) {
    struct Token *ret_token = parser_expect(parser, TOK_KW_RETURN, "Expected 'return'");
    struct Tree *ret_node = tree_new_node(1, ret_token);

    ret_node->nodes[0] = parse_expression_until(parser, TOK_SEMICOLON);
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after return expression");

    return ret_node;
}

static struct Tree *parse_expression_statement(struct AstParser *parser) {
    struct Tree *expr_node = parse_expression_until(parser, TOK_SEMICOLON);
    parser_expect(parser, TOK_SEMICOLON, "Expected ';' after expression statement");
    return expr_node;
}

static struct Tree *parse_statement(struct AstParser *parser) {
    struct Token *tok = parser_peek(parser);
    if (tok == NULL) {
        return tree_new_node(0, NULL);
    }

    switch (tok->kind) {
    case TOK_KW_IF:
        return parse_if_statement(parser);
    case TOK_KW_WHILE:
        return parse_while_statement(parser);
    case TOK_KW_FOR:
        return parse_for_statement(parser);
    case TOK_KW_VAR:
        return parse_var_statement(parser);
    case TOK_KW_RETURN:
        return parse_return_statement(parser);
    case TOK_LBRACE: {
        struct Token *lbrace = parser_advance(parser);
        return parse_block_body(parser, lbrace);
    }
    default:
        return parse_expression_statement(parser);
    }
}

static struct Tree *parse_function(struct AstParser *parser) {
    parser_expect(parser, TOK_KW_FUN, "Expected 'fun'");

    struct Token *name_token = parser_expect(parser, TOK_IDENT, "Expected function name after 'fun'");
    struct Tree *func_node = tree_new_node(3, name_token);

    if (parser_expect(parser, TOK_LPAREN, "Expected '(' after function name") != NULL) {
        size_t params_start = parser->pos;
        size_t depth = 1;
        while (!parser_is_at_end(parser) && depth > 0) {
            struct Token *tok = parser_advance(parser);
            if (tok == NULL) {
                break;
            }
            if (tok->kind == TOK_LPAREN) {
                depth++;
            } else if (tok->kind == TOK_RPAREN) {
                depth--;
            }
        }

        if (depth == 0 && parser->pos > 0) {
            size_t params_end_exclusive = parser->pos - 1;
            func_node->nodes[0] = build_expression_from_range(parser->tokens, params_start, params_end_exclusive);
        } else {
            fprintf(stderr, "AST parse error: Unterminated function parameter list\n");
        }
    }

    while (!parser_is_at_end(parser)) {
        struct Token *tok = parser_peek(parser);
        if (tok == NULL) {
            break;
        }
        if (tok->kind == TOK_LBRACE) {
            struct Token *lbrace = parser_advance(parser);
            func_node->nodes[1] = parse_block_body(parser, lbrace);
            break;
        }
        parser_advance(parser);
    }

    return func_node;
}

static void append_function_sibling(struct Tree *root_function, struct Tree *other_function) {
    if (root_function == NULL || other_function == NULL) {
        return;
    }

    if (root_function->branch_count < 3) {
        return;
    }

    if (root_function->nodes[2] == NULL) {
        root_function->nodes[2] = other_function;
        return;
    }

    struct Tree *cursor = root_function->nodes[2];
    while (cursor->branch_count >= 3 && cursor->nodes[2] != NULL) {
        cursor = cursor->nodes[2];
    }
    if (cursor->branch_count >= 3) {
        cursor->nodes[2] = other_function;
    }
}

struct Tree build_tree(struct dynamic *tokens) {
    if (tokens == NULL || tokens->size == 0) {
        return init_tree(0, NULL);
    }

    struct AstParser parser = {.tokens = tokens, .pos = 0};

    struct Tree **functions = NULL;
    size_t function_count = 0;
    size_t function_capacity = 0;
    struct Tree *main_function = NULL;

    while (!parser_is_at_end(&parser)) {
        struct Token *tok = parser_peek(&parser);
        if (tok == NULL) {
            break;
        }

        if (tok->kind == TOK_KW_FUN) {
            struct Tree *function = parse_function(&parser);
            if (function != NULL) {
                if (function_count == function_capacity) {
                    size_t new_capacity = function_capacity == 0 ? 4 : function_capacity * 2;
                    struct Tree **new_functions = realloc(functions, sizeof(struct Tree *) * new_capacity);
                    if (new_functions == NULL) {
                        fprintf(stderr, "Memory allocation failed\n");
                        exit(1);
                    }
                    functions = new_functions;
                    function_capacity = new_capacity;
                }
                functions[function_count++] = function;
                if (function->token != NULL && strcmp(function->token->lexeme, "main") == 0) {
                    main_function = function;
                }
            }
            continue;
        }

        parser_advance(&parser);
    }

    if (main_function == NULL) {
        fprintf(stderr, "AST parse error: main function not found\n");
        for (size_t i = 0; i < function_count; i++) {
            if (functions[i] != NULL) {
                tree_free(functions[i]);
                free(functions[i]);
            }
        }
        free(functions);
        return init_tree(0, NULL);
    }

    struct Tree result = *main_function;
    free(main_function);

    for (size_t i = 0; i < function_count; i++) {
        struct Tree *function = functions[i];
        if (function == NULL || function == main_function) {
            continue;
        }
        append_function_sibling(&result, function);
    }
    free(functions);

    return result;
}
