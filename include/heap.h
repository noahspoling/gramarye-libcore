/* $Id$ */
#ifndef HEAP_INCLUDED
#define HEAP_INCLUDED
#define T Heap_T
typedef struct T *T;

/* A binary heap with value semantics. Elements are fixed-size blobs copied in
   and out by value (like Array_T's (length,size)), so callers can store small
   structs -- e.g. { int cell; float key; } -- inline, with no per-element
   allocation even over hundreds of thousands of pushes.

   `cmp(a,b)` orders the heap: the element for which cmp sorts smallest rises to
   the top, so a "less-than" comparator makes a MIN-heap (Heap_pop returns the
   least element first). */

extern T    Heap_new (int hint, int size,
                      int (*cmp)(const void *a, const void *b));
extern void Heap_free(T *heap);

extern int  Heap_length(T heap);
extern void Heap_push  (T heap, const void *elem); /* copies `size` bytes in */
extern int  Heap_pop   (T heap, void *out);        /* copies top out; 0 if empty */
extern const void *Heap_peek(T heap);              /* top, or NULL if empty */
extern void Heap_clear (T heap);                   /* length -> 0, keep capacity */

#undef T
#endif
