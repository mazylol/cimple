#ifndef DYNAMIC_H
#define DYNAMIC_H

#include <stddef.h>

struct dynamic {
    void **elements;
    size_t size;
    size_t capacity;
    size_t typeSize;
};

struct dynamic dynamic_init(int type_size);

void dynamic_push(struct dynamic *dyn, const void *element);

void dynamic_free(const struct dynamic *dyn);

#endif // DYNAMIC_H
