/* $Id$ */
#ifndef KDTREE_INCLUDED
#define KDTREE_INCLUDED
#define T KDTree_T
typedef struct T *T;

/* A static k-d tree over n `dim`-dimensional float points, for nearest-K and
   radius queries. Built once from a snapshot of the points (copied in);
   rebuild (KDTree_new again) if the points move.

   Points are embedded in ordinary Euclidean space -- there is nothing
   sphere-specific here. For points on a unit sphere (planet cell positions,
   control points on a coarse field, ...) that is fine as-is: chord distance
   is monotonic with great-circle distance, so plain Euclidean nearest-
   neighbor / radius queries over (x,y,z) already give the correct answer
   with no spherical math needed.

   Queries report the ORIGINAL index into the `points` array passed to
   KDTree_new (0..n-1) -- there is no separate id concept, so the caller's own
   index (into a control-point array, a cell array, ...) doubles as the id. */

extern T   KDTree_new (int n, int dim, const float *points /* n*dim floats, row-major */);
extern void KDTree_free(T *t);

extern int KDTree_count(T t);

/* K nearest neighbors of `query` (dim floats). Writes up to k point indices
   into out_idx, nearest first; if out_dist2 is non-NULL, writes the matching
   squared distances alongside. Returns the number written (<= k, <= point
   count). */
extern int KDTree_nearest(T t, const float *query, int k,
                          int *out_idx, float *out_dist2);

/* All point indices within `radius` of `query`. Writes up to max_out indices
   into out_idx, unordered. Returns the number written -- if more points
   match than max_out, the rest are silently dropped (size max_out
   generously; this bounds the scratch buffer, it is not a query filter). */
extern int KDTree_radius(T t, const float *query, float radius,
                         int *out_idx, int max_out);

#undef T
#endif
