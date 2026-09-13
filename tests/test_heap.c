#include "test_common.h"
#include "heap.h"
#include "except.h"
#include <string.h>
#include <stdlib.h>

typedef struct { int cell; float key; } Item;

static int item_cmp(const void *a, const void *b) {
    const Item *ia = (const Item *)a;
    const Item *ib = (const Item *)b;
    if (ia->key < ib->key) return -1;
    if (ia->key > ib->key) return 1;
    return 0;
}

static int int_min_cmp(const void *a, const void *b) {
    int ia = *(const int *)a;
    int ib = *(const int *)b;
    return ia - ib;
}

static void test_empty_heap(void) {
    printf("  Testing empty heap behavior...\n");

    Heap_T heap = Heap_new(0, sizeof(int), int_min_cmp);
    TEST_ASSERT_NOT_NULL(heap, "Heap creation failed");
    TEST_ASSERT_EQ(Heap_length(heap), 0, "New heap should be empty");
    TEST_ASSERT_NULL(Heap_peek(heap), "Peek on empty heap should return NULL");

    int out = -1;
    int popped = Heap_pop(heap, &out);
    TEST_ASSERT_EQ(popped, 0, "Pop on empty heap should return 0");
    TEST_ASSERT_EQ(out, -1, "Pop on empty heap should not touch out param");

    Heap_free(&heap);
}

static void test_single_push_pop(void) {
    printf("  Testing single push/pop...\n");

    Heap_T heap = Heap_new(4, sizeof(int), int_min_cmp);
    int value = 42;
    Heap_push(heap, &value);
    TEST_ASSERT_EQ(Heap_length(heap), 1, "Heap length should be 1 after one push");

    const int *top = (const int *)Heap_peek(heap);
    TEST_ASSERT_NOT_NULL(top, "Peek should not be NULL after push");
    TEST_ASSERT_EQ(*top, 42, "Peek should return pushed value");

    int out = 0;
    int popped = Heap_pop(heap, &out);
    TEST_ASSERT_EQ(popped, 1, "Pop should succeed");
    TEST_ASSERT_EQ(out, 42, "Popped value should match pushed value");
    TEST_ASSERT_EQ(Heap_length(heap), 0, "Heap should be empty after popping only element");

    Heap_free(&heap);
}

static void test_min_heap_ordering(void) {
    printf("  Testing min-heap ordering across many pushes...\n");

    int values[] = { 5, 3, 8, 1, 9, 2, 7, 4, 6, 0, 100, -5, 42, 17, 23 };
    int n = (int)(sizeof(values) / sizeof(values[0]));

    /* Start with a small hint to force at least one grow via RESIZE. */
    Heap_T heap = Heap_new(2, sizeof(int), int_min_cmp);
    for (int i = 0; i < n; i++) {
        Heap_push(heap, &values[i]);
    }
    TEST_ASSERT_EQ(Heap_length(heap), n, "Heap length should equal number pushed");

    int sorted[15];
    memcpy(sorted, values, sizeof(values));
    /* simple insertion sort for the expected order */
    for (int i = 1; i < n; i++) {
        int key = sorted[i], j = i - 1;
        while (j >= 0 && sorted[j] > key) {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = key;
    }

    for (int i = 0; i < n; i++) {
        int out;
        int popped = Heap_pop(heap, &out);
        TEST_ASSERT_EQ(popped, 1, "Pop should succeed while heap non-empty");
        TEST_ASSERT_EQ(out, sorted[i], "Pop order should follow ascending sort (min-heap)");
    }
    TEST_ASSERT_EQ(Heap_length(heap), 0, "Heap should be empty after popping everything");

    Heap_free(&heap);
}

static void test_struct_elements(void) {
    printf("  Testing heap with struct elements (value semantics)...\n");

    Heap_T heap = Heap_new(4, sizeof(Item), item_cmp);
    Item items[] = {
        { 10, 3.5f },
        { 20, 1.5f },
        { 30, 2.5f },
    };
    for (int i = 0; i < 3; i++) Heap_push(heap, &items[i]);

    Item out;
    Heap_pop(heap, &out);
    TEST_ASSERT_EQ(out.cell, 20, "Lowest key item should pop first");

    Heap_pop(heap, &out);
    TEST_ASSERT_EQ(out.cell, 30, "Second lowest key item should pop second");

    Heap_pop(heap, &out);
    TEST_ASSERT_EQ(out.cell, 10, "Highest key item should pop last");

    Heap_free(&heap);
}

static void test_clear_and_reuse(void) {
    printf("  Testing Heap_clear preserves capacity and allows reuse...\n");

    Heap_T heap = Heap_new(4, sizeof(int), int_min_cmp);
    for (int i = 0; i < 10; i++) {
        int v = 10 - i;
        Heap_push(heap, &v);
    }
    TEST_ASSERT_EQ(Heap_length(heap), 10, "Heap should have 10 elements before clear");

    Heap_clear(heap);
    TEST_ASSERT_EQ(Heap_length(heap), 0, "Heap length should be 0 after clear");
    TEST_ASSERT_NULL(Heap_peek(heap), "Peek should be NULL after clear");

    int v = 99;
    Heap_push(heap, &v);
    TEST_ASSERT_EQ(Heap_length(heap), 1, "Heap should accept pushes after clear");
    const int *top = (const int *)Heap_peek(heap);
    TEST_ASSERT_EQ(*top, 99, "Peek should reflect post-clear push");

    Heap_free(&heap);
}

static void test_pop_without_out_param(void) {
    printf("  Testing Heap_pop with NULL out param just discards top...\n");

    Heap_T heap = Heap_new(4, sizeof(int), int_min_cmp);
    int a = 5, b = 1, c = 3;
    Heap_push(heap, &a);
    Heap_push(heap, &b);
    Heap_push(heap, &c);

    int popped = Heap_pop(heap, NULL);
    TEST_ASSERT_EQ(popped, 1, "Pop with NULL out should still report success");
    TEST_ASSERT_EQ(Heap_length(heap), 2, "Heap length should decrease after pop");

    int out;
    Heap_pop(heap, &out);
    TEST_ASSERT_EQ(out, 3, "Next min after removing 1 should be 3");

    Heap_free(&heap);
}

bool test_heap(void) {
    printf("\n=== Heap Tests ===\n");

    bool result = true;
    TRY
        test_empty_heap();
        test_single_push_pop();
        test_min_heap_ordering();
        test_struct_elements();
        test_clear_and_reuse();
        test_pop_without_out_param();
    EXCEPT(Test_Failed)
        result = false;
    END_TRY;

    return result;
}
