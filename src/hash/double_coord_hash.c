#include "double_coord_hash.h"
#include <stdint.h>
#include <string.h>

int double_coord_cmp(const void *x, const void *y) {
    const double_coord_t *a = (const double_coord_t *)x;
    const double_coord_t *b = (const double_coord_t *)y;
    return (a->x != b->x) || (a->y != b->y);
}

unsigned double_coord_hash(const void *key) {
    const double_coord_t *coord = (const double_coord_t *)key;
    unsigned hash = 2166136261u;
    uint64_t x_bits, y_bits;
    memcpy(&x_bits, &coord->x, sizeof x_bits);
    memcpy(&y_bits, &coord->y, sizeof y_bits);
    hash = (hash ^ (unsigned)(x_bits ^ (x_bits >> 32))) * 16777619u;
    hash = (hash ^ (unsigned)(y_bits ^ (y_bits >> 32))) * 16777619u;
    return hash;
}