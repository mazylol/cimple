#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/ast.h"
#include "../include/dynamic.h"
#include "../include/lexer.h"

int main(int argc, char **argv) {
    int arg_index = 1;
    int show_legend = 0;

    if (argc < 2) {
        printf("Usage: cimple [--legend] <file.cimple>\n");
        exit(1);
    }

    if (strcmp(argv[arg_index], "--legend") == 0) {
        show_legend = 1;
        arg_index++;
    }

    if (arg_index >= argc) {
        printf("Usage: cimple [--legend] <file.cimple>\n");
        exit(1);
    }

    FILE *fp = fopen(argv[arg_index], "r");

    if (fp == NULL) {
        perror("Error opening file");
        return EXIT_FAILURE;
    }

    struct dynamic tokens = get_tokens(fp);

    if (show_legend) {
        print_token_legend();
        printf("\n");
    }

    token_debug_print(&tokens);

    fclose(fp);

    struct Tree tree = build_tree(&tokens);

    tree_debug_print(&tree);

    // for the interpreter, should create a variable hashmap. ident -> [type, value]
    // interpret(&tree);

    dynamic_free(&tokens);

    return 0;
}
