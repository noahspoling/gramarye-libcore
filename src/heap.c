static const char *rcsid = "$Id$";
#include <stdlib.h>
#include <string.h>
#include "assert.h"
#include "heap.h"
#include "mem.h"
#define T Heap_T

struct T {
	char *data;    /* count elements of `size` bytes, contiguous */
	char *scratch; /* one element of scratch, for swaps */
	int   count;
	int   cap;     /* capacity in elements */
	int   size;    /* bytes per element */
	int  (*cmp)(const void *a, const void *b);
};

/* address of element i */
#define AT(h, i) ((h)->data + (long)(i) * (h)->size)

T Heap_new(int hint, int size, int (*cmp)(const void *a, const void *b)) {
	T heap;
	assert(hint >= 0);
	assert(size > 0);
	assert(cmp);
	NEW(heap);
	heap->size    = size;
	heap->cmp     = cmp;
	heap->count   = 0;
	heap->cap     = hint > 0 ? hint : 16;
	heap->data    = ALLOC((long)heap->cap * size);
	heap->scratch = ALLOC((long)size);
	return heap;
}

void Heap_free(T *heap) {
	assert(heap && *heap);
	FREE((*heap)->data);
	FREE((*heap)->scratch);
	FREE(*heap);
}

int  Heap_length(T heap) { assert(heap); return heap->count; }
void Heap_clear (T heap) { assert(heap); heap->count = 0; }

static void swap(T h, int i, int j) {
	memcpy(h->scratch, AT(h, i), h->size);
	memcpy(AT(h, i),   AT(h, j), h->size);
	memcpy(AT(h, j),   h->scratch, h->size);
}

static void sift_up(T h, int i) {
	while (i > 0) {
		int parent = (i - 1) / 2;
		if (h->cmp(AT(h, i), AT(h, parent)) < 0) {
			swap(h, i, parent);
			i = parent;
		} else break;
	}
}

static void sift_down(T h, int i) {
	int n = h->count;
	for (;;) {
		int l = 2 * i + 1, r = 2 * i + 2, best = i;
		if (l < n && h->cmp(AT(h, l), AT(h, best)) < 0) best = l;
		if (r < n && h->cmp(AT(h, r), AT(h, best)) < 0) best = r;
		if (best == i) break;
		swap(h, i, best);
		i = best;
	}
}

void Heap_push(T heap, const void *elem) {
	assert(heap && elem);
	if (heap->count == heap->cap) {
		heap->cap *= 2;
		RESIZE(heap->data, (long)heap->cap * heap->size);
	}
	memcpy(AT(heap, heap->count), elem, heap->size);
	heap->count++;
	sift_up(heap, heap->count - 1);
}

int Heap_pop(T heap, void *out) {
	assert(heap);
	if (heap->count == 0) return 0;
	if (out) memcpy(out, AT(heap, 0), heap->size);
	heap->count--;
	if (heap->count > 0) {
		memcpy(AT(heap, 0), AT(heap, heap->count), heap->size);
		sift_down(heap, 0);
	}
	return 1;
}

const void *Heap_peek(T heap) {
	assert(heap);
	return heap->count > 0 ? (const void *)AT(heap, 0) : NULL;
}
