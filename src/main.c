#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    struct dynamic tokens = dynamic_init(sizeof(struct Token));

    while (1) {
        struct Token tok = next_token(fp);
        dynamic_push(&tokens, &tok);

        if (tok.kind == TOK_EOF) {
            break;
        }
    }

    if (show_legend) {
        print_token_legend();
        printf("\n");
    }

    for (int i = 0; i < tokens.size; i++) {
        struct Token *tok = (struct Token *)tokens.elements[i];
        printf("%-16s -> %s", token_enum_name(tok->kind), token_debug_mapping(tok->kind));
        if (tok->lexeme[0] != '\0') {
            printf(" (lexeme: %s)", tok->lexeme);
        }
        printf("\n");
    }

    fclose(fp);
    dynamic_free(&tokens);

    return 0;
}
