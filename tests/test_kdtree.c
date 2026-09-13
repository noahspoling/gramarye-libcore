#include "test_common.h"
#include "kdtree.h"
#include "except.h"
#include <stdlib.h>
#include <math.h>

/* Brute-force nearest-k over a 2D point set, for cross-checking KDTree_nearest. */
static void brute_nearest(const float *pts, int n, int dim, const float *q, int k,
                           int *out_idx, float *out_dist2) {
    typedef struct { int idx; float d2; } Cand;
    Cand *cands = malloc(sizeof(Cand) * (size_t)n);
    for (int i = 0; i < n; i++) {
        float sum = 0.0f;
        for (int d = 0; d < dim; d++) {
            float diff = q[d] - pts[i * dim + d];
            sum += diff * diff;
        }
        cands[i].idx = i;
        cands[i].d2 = sum;
    }
    /* simple selection sort by distance, stable-ish for ties on index */
    for (int i = 0; i < n && i < k; i++) {
        int best = i;
        for (int j = i + 1; j < n; j++) {
            if (cands[j].d2 < cands[best].d2) best = j;
        }
        Cand tmp = cands[i]; cands[i] = cands[best]; cands[best] = tmp;
        out_idx[i] = cands[i].idx;
        if (out_dist2) out_dist2[i] = cands[i].d2;
    }
    free(cands);
}

static void test_empty_tree(void) {
    printf("  Testing empty kd-tree...\n");

    KDTree_T t = KDTree_new(0, 2, NULL);
    TEST_ASSERT_NOT_NULL(t, "KDTree_new with n=0 should still return a valid handle");
    TEST_ASSERT_EQ(KDTree_count(t), 0, "Empty tree count should be 0");

    float q[2] = { 0.0f, 0.0f };
    int idx[4];
    int found = KDTree_nearest(t, q, 3, idx, NULL);
    TEST_ASSERT_EQ(found, 0, "Nearest on empty tree should return 0");

    found = KDTree_radius(t, q, 100.0f, idx, 4);
    TEST_ASSERT_EQ(found, 0, "Radius query on empty tree should return 0");

    KDTree_free(&t);
}

static void test_single_point(void) {
    printf("  Testing single-point tree...\n");

    float pts[2] = { 3.0f, 4.0f };
    KDTree_T t = KDTree_new(1, 2, pts);
    TEST_ASSERT_EQ(KDTree_count(t), 1, "Single-point tree count should be 1");

    float q[2] = { 0.0f, 0.0f };
    int idx[1];
    float d2[1];
    int found = KDTree_nearest(t, q, 1, idx, d2);
    TEST_ASSERT_EQ(found, 1, "Nearest should find the one point");
    TEST_ASSERT_EQ(idx[0], 0, "Nearest index should be 0");
    TEST_ASSERT_TRUE(fabsf(d2[0] - 25.0f) < 1e-4f, "Distance^2 to (3,4) from origin should be 25");

    KDTree_free(&t);
}

static void test_nearest_matches_brute_force(void) {
    printf("  Testing KDTree_nearest matches brute force on random-ish point set...\n");

    /* deterministic pseudo-random point set */
    const int n = 200, dim = 2;
    float *pts = malloc(sizeof(float) * (size_t)n * dim);
    unsigned seed = 12345u;
    for (int i = 0; i < n * dim; i++) {
        seed = seed * 1103515245u + 12345u;
        pts[i] = (float)((seed >> 8) % 10000) / 100.0f - 50.0f;
    }

    KDTree_T t = KDTree_new(n, dim, pts);
    TEST_ASSERT_EQ(KDTree_count(t), n, "Tree count should match n");

    float queries[5][2] = {
        { 0.0f, 0.0f }, { 25.0f, -10.0f }, { -40.0f, 40.0f }, { 5.5f, 5.5f }, { -50.0f, -50.0f }
    };

    for (int qi = 0; qi < 5; qi++) {
        int k = 5;
        int tree_idx[5];
        float tree_d2[5];
        int tree_found = KDTree_nearest(t, queries[qi], k, tree_idx, tree_d2);
        TEST_ASSERT_EQ(tree_found, k, "KDTree should return k results when n >= k");

        int brute_idx[5];
        float brute_d2[5];
        brute_nearest(pts, n, dim, queries[qi], k, brute_idx, brute_d2);

        for (int i = 0; i < k; i++) {
            TEST_ASSERT_TRUE(fabsf(tree_d2[i] - brute_d2[i]) < 1e-3f,
                "KDTree nearest distance should match brute-force distance at each rank");
        }
        /* Nearest-first ordering must hold */
        for (int i = 1; i < k; i++) {
            TEST_ASSERT_TRUE(tree_d2[i - 1] <= tree_d2[i] + 1e-6f,
                "KDTree nearest results must be sorted nearest-first");
        }
    }

    KDTree_free(&t);
    free(pts);
}

static void test_radius_matches_brute_force(void) {
    printf("  Testing KDTree_radius matches brute force point set (as unordered sets)...\n");

    const int n = 100, dim = 2;
    float pts[200];
    unsigned seed = 999u;
    for (int i = 0; i < n * dim; i++) {
        seed = seed * 1103515245u + 12345u;
        pts[i] = (float)((seed >> 8) % 2000) / 100.0f - 10.0f;
    }

    KDTree_T t = KDTree_new(n, dim, pts);

    float q[2] = { 0.0f, 0.0f };
    float radius = 5.0f;

    int tree_idx[100];
    int tree_found = KDTree_radius(t, q, radius, tree_idx, 100);

    /* brute force: which indices fall within radius */
    int expected_count = 0;
    char expected_present[100] = {0};
    for (int i = 0; i < n; i++) {
        float dx = q[0] - pts[i * dim + 0];
        float dy = q[1] - pts[i * dim + 1];
        float d2 = dx * dx + dy * dy;
        if (d2 <= radius * radius) {
            expected_present[i] = 1;
            expected_count++;
        }
    }

    TEST_ASSERT_EQ(tree_found, expected_count, "Radius query count should match brute force count");

    char tree_present[100] = {0};
    for (int i = 0; i < tree_found; i++) {
        TEST_ASSERT_TRUE(tree_idx[i] >= 0 && tree_idx[i] < n, "Radius result index should be in range");
        tree_present[tree_idx[i]] = 1;
    }
    for (int i = 0; i < n; i++) {
        TEST_ASSERT_EQ(tree_present[i], expected_present[i], "Radius result set should match brute-force set");
    }

    KDTree_free(&t);
}

static void test_radius_max_out_bound(void) {
    printf("  Testing KDTree_radius respects max_out bound...\n");

    float pts[10] = { 0,0, 1,0, 2,0, 3,0, 4,0 };
    KDTree_T t = KDTree_new(5, 2, pts);

    float q[2] = { 0.0f, 0.0f };
    int idx[2];
    int found = KDTree_radius(t, q, 100.0f, idx, 2);
    TEST_ASSERT_EQ(found, 2, "Radius query should be bounded by max_out even if more points match");

    KDTree_free(&t);
}

static void test_nearest_k_larger_than_n(void) {
    printf("  Testing KDTree_nearest with k larger than point count...\n");

    float pts[6] = { 0,0, 1,1, 2,2 };
    KDTree_T t = KDTree_new(3, 2, pts);

    float q[2] = { 0.0f, 0.0f };
    int idx[10];
    float d2[10];
    int found = KDTree_nearest(t, q, 10, idx, d2);
    TEST_ASSERT_EQ(found, 3, "Nearest should return at most n results when k > n");

    KDTree_free(&t);
}

bool test_kdtree(void) {
    printf("\n=== KDTree Tests ===\n");

    bool result = true;
    TRY
        test_empty_tree();
        test_single_point();
        test_nearest_matches_brute_force();
        test_radius_matches_brute_force();
        test_radius_max_out_bound();
        test_nearest_k_larger_than_n();
    EXCEPT(Test_Failed)
        result = false;
    END_TRY;

    return result;
}
