#include "../include/dynamic.h"

#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

struct dynamic dynamic_init(const int typeSize) {
    struct dynamic dyn = {};

    dyn.elements = malloc(typeSize * 8);
    dyn.size = 0;
    dyn.capacity = 4;
    dyn.typeSize = typeSize * 2;

    return dyn;
}

void dynamic_free(const struct dynamic *dyn) {
    for (int i = 0; i < dyn->size; i++) {
        free(dyn->elements[i]);
    }

    free(dyn->elements);
}

void dynamic_push(struct dynamic *dyn, const void *element) {
    dyn->size++;

    if (dyn->size > dyn->capacity) {
        dyn->capacity *= 2;
        dyn->elements = realloc(dyn->elements, dyn->capacity * dyn->typeSize);
    }

    if (dyn->elements == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    dyn->elements[dyn->size - 1] = malloc(dyn->typeSize);
    memcpy(dyn->elements[dyn->size - 1], element, dyn->typeSize);
}
