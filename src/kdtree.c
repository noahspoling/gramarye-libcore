static const char *rcsid = "$Id$";
#include <stdlib.h>
#include <string.h>
#include "assert.h"
#include "kdtree.h"
#include "mem.h"
#define T KDTree_T

struct T {
	int n, dim;
	float *pts;    /* n*dim floats, row-major, copied from the caller */
	int   *pidx;   /* node id -> point index (into pts / the caller's array) */
	int   *left;   /* node id -> left child node id, or -1 */
	int   *right;  /* node id -> right child node id, or -1 */
	int   *ax;     /* node id -> split axis for this node */
	int    root;   /* root node id, or -1 if empty */
};

#define COORD(t, p, d) ((t)->pts[(long)(p) * (t)->dim + (d)])

/* Partial quickselect: after this, idx[mid] holds the point whose coordinate
   at `axis` is the median of idx[lo,hi), everything before it <=, everything
   after it >=. That is all a kd-tree build needs -- not a full sort. */
static void select_median(T t, int *idx, int lo, int hi, int mid, int axis) {
	while (hi - lo > 1) {
		int pivot_i = lo + (hi - lo) / 2;
		float pivot = COORD(t, idx[pivot_i], axis);
		int tmp, store, i;
		tmp = idx[pivot_i]; idx[pivot_i] = idx[hi - 1]; idx[hi - 1] = tmp;
		store = lo;
		for (i = lo; i < hi - 1; i++) {
			if (COORD(t, idx[i], axis) < pivot) {
				tmp = idx[i]; idx[i] = idx[store]; idx[store] = tmp;
				store++;
			}
		}
		tmp = idx[store]; idx[store] = idx[hi - 1]; idx[hi - 1] = tmp;
		if (store == mid) return;
		else if (store < mid) lo = store + 1;
		else hi = store;
	}
}

/* Recursive median-split build. Node ids are handed out in build (pre-)order
   via `next_node`; there is one node per point, so ids fit in [0, n). */
static int build(T t, int *idx, int lo, int hi, int depth, int *next_node) {
	int axis, mid, node;
	if (lo >= hi) return -1;
	axis = depth % t->dim;
	mid  = lo + (hi - lo) / 2;
	select_median(t, idx, lo, hi, mid, axis);
	node = (*next_node)++;
	t->pidx[node] = idx[mid];
	t->ax[node]   = axis;
	t->left[node]  = build(t, idx, lo, mid, depth + 1, next_node);
	t->right[node] = build(t, idx, mid + 1, hi, depth + 1, next_node);
	return node;
}

T KDTree_new(int n, int dim, const float *points) {
	T t;
	assert(n >= 0);
	assert(dim > 0);
	assert(n == 0 || points);
	NEW(t);
	t->n = n;
	t->dim = dim;
	t->root = -1;
	t->pts = NULL;
	t->pidx = t->left = t->right = t->ax = NULL;
	if (n > 0) {
		int *idx, i, next_node = 0;
		t->pts = ALLOC((long)n * dim * sizeof(float));
		memcpy(t->pts, points, (size_t)n * dim * sizeof(float));
		t->pidx  = ALLOC((long)n * sizeof(int));
		t->left  = ALLOC((long)n * sizeof(int));
		t->right = ALLOC((long)n * sizeof(int));
		t->ax    = ALLOC((long)n * sizeof(int));
		idx = ALLOC((long)n * sizeof(int));
		for (i = 0; i < n; i++) idx[i] = i;
		t->root = build(t, idx, 0, n, 0, &next_node);
		FREE(idx);
	}
	return t;
}

void KDTree_free(T *t) {
	assert(t && *t);
	FREE((*t)->pts);
	FREE((*t)->pidx);
	FREE((*t)->left);
	FREE((*t)->right);
	FREE((*t)->ax);
	FREE(*t);
}

int KDTree_count(T t) { assert(t); return t->n; }

static float point_dist2(T t, const float *q, int p) {
	float sum = 0.0f;
	int d;
	for (d = 0; d < t->dim; d++) {
		float diff = q[d] - COORD(t, p, d);
		sum += diff * diff;
	}
	return sum;
}

/* Bounded "top-k smallest" via insertion into a sorted-ascending array --
   cheap and simple for the small k (a handful of control points) this is
   built for; not intended for large k. */
typedef struct { int idx; float d2; } Cand;

static void knn_insert(Cand *cands, int *count, int k, int idx, float d2) {
	int i;
	if (*count < k) {
		i = (*count)++;
	} else if (d2 < cands[k - 1].d2) {
		i = k - 1;
	} else {
		return;
	}
	for (; i > 0 && cands[i - 1].d2 > d2; i--) cands[i] = cands[i - 1];
	cands[i].idx = idx;
	cands[i].d2  = d2;
}

static void knn_recurse(T t, const float *q, int node, Cand *cands, int *count, int k) {
	int p, axis, near, far;
	float diff;
	if (node < 0) return;
	p = t->pidx[node];
	knn_insert(cands, count, k, p, point_dist2(t, q, p));

	axis = t->ax[node];
	diff = q[axis] - COORD(t, p, axis);
	near = diff < 0.0f ? t->left[node] : t->right[node];
	far  = diff < 0.0f ? t->right[node] : t->left[node];
	knn_recurse(t, q, near, cands, count, k);
	/* Only the far side can hold anything closer than what we already have
	   if the split plane itself is nearer than our current worst kept
	   candidate -- standard kd-tree pruning. */
	if (*count < k || diff * diff < cands[*count - 1].d2)
		knn_recurse(t, q, far, cands, count, k);
}

int KDTree_nearest(T t, const float *query, int k, int *out_idx, float *out_dist2) {
	Cand *cands;
	int count = 0, i;
	assert(t && query && out_idx);
	if (k <= 0 || t->n == 0) return 0;
	cands = ALLOC((long)k * sizeof(Cand));
	knn_recurse(t, query, t->root, cands, &count, k);
	for (i = 0; i < count; i++) {
		out_idx[i] = cands[i].idx;
		if (out_dist2) out_dist2[i] = cands[i].d2;
	}
	FREE(cands);
	return count;
}

static void radius_recurse(T t, const float *q, int node, float r2,
                           int *out_idx, int max_out, int *count) {
	int p, axis;
	float diff;
	if (node < 0 || *count >= max_out) return;
	p = t->pidx[node];
	if (point_dist2(t, q, p) <= r2 && *count < max_out)
		out_idx[(*count)++] = p;

	axis = t->ax[node];
	diff = q[axis] - COORD(t, p, axis);
	if (diff <= 0.0f) {
		radius_recurse(t, q, t->left[node],  r2, out_idx, max_out, count);
		if (diff * diff <= r2)
			radius_recurse(t, q, t->right[node], r2, out_idx, max_out, count);
	} else {
		radius_recurse(t, q, t->right[node], r2, out_idx, max_out, count);
		if (diff * diff <= r2)
			radius_recurse(t, q, t->left[node],  r2, out_idx, max_out, count);
	}
}

int KDTree_radius(T t, const float *query, float radius, int *out_idx, int max_out) {
	int count = 0;
	assert(t && query && out_idx);
	if (max_out <= 0 || t->n == 0) return 0;
	radius_recurse(t, query, t->root, radius * radius, out_idx, max_out, &count);
	return count;
}
