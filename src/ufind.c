static const char *rcsid = "$Id$";
#include <stdlib.h>
#include "assert.h"
#include "ufind.h"
#include "mem.h"
#define T UFind_T

struct T {
	int *parent;
	int *size;
	int  n;    /* element count */
	int  sets; /* number of disjoint sets */
};

static void init(T uf, int n) {
	int i;
	for (i = 0; i < n; i++) {
		uf->parent[i] = i;
		uf->size[i]   = 1;
	}
	uf->n    = n;
	uf->sets = n;
}

T UFind_new(int n) {
	T uf;
	assert(n >= 0);
	NEW(uf);
	uf->parent = n > 0 ? ALLOC((long)n * sizeof(int)) : NULL;
	uf->size   = n > 0 ? ALLOC((long)n * sizeof(int)) : NULL;
	init(uf, n);
	return uf;
}

void UFind_free(T *uf) {
	assert(uf && *uf);
	FREE((*uf)->parent);
	FREE((*uf)->size);
	FREE(*uf);
}

int UFind_find(T uf, int x) {
	assert(uf);
	assert(x >= 0 && x < uf->n);
	while (uf->parent[x] != x) {
		uf->parent[x] = uf->parent[uf->parent[x]]; /* path halving */
		x = uf->parent[x];
	}
	return x;
}

int UFind_union(T uf, int a, int b) {
	int ra = UFind_find(uf, a), rb = UFind_find(uf, b);
	if (ra == rb) return ra;
	if (uf->size[ra] < uf->size[rb]) { int t = ra; ra = rb; rb = t; }
	uf->parent[rb] = ra;
	uf->size[ra]  += uf->size[rb];
	uf->sets--;
	return ra;
}

int UFind_connected(T uf, int a, int b) {
	return UFind_find(uf, a) == UFind_find(uf, b);
}

int UFind_size(T uf, int x) {
	return uf->size[UFind_find(uf, x)];
}

int UFind_count(T uf) {
	assert(uf);
	return uf->sets;
}

void UFind_reset(T uf, int n) {
	assert(uf);
	assert(n >= 0);
	/* Mem_resize asserts a non-NULL ptr, so free + alloc rather than resize. */
	FREE(uf->parent);
	FREE(uf->size);
	uf->parent = n > 0 ? ALLOC((long)n * sizeof(int)) : NULL;
	uf->size   = n > 0 ? ALLOC((long)n * sizeof(int)) : NULL;
	init(uf, n);
}
