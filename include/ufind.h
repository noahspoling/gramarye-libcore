/* $Id$ */
#ifndef UFIND_INCLUDED
#define UFIND_INCLUDED
#define T UFind_T
typedef struct T *T;

/* Index-based disjoint-set / union-find over elements 0..n-1, with path
   halving and union by size. Purpose-built for cell-indexed connected
   components: landmasses, drainage basins, regions -- merge neighbors, then
   read each element's representative. */

extern T    UFind_new (int n);          /* n singletons {0}..{n-1} */
extern void UFind_free(T *uf);

extern int  UFind_find     (T uf, int x);        /* representative of x's set */
extern int  UFind_union    (T uf, int a, int b); /* merge a,b; returns new root */
extern int  UFind_connected(T uf, int a, int b); /* 1 if same set */
extern int  UFind_size     (T uf, int x);        /* size of x's set */
extern int  UFind_count    (T uf);               /* number of disjoint sets */
extern void UFind_reset    (T uf, int n);        /* re-init to n singletons */

#undef T
#endif
