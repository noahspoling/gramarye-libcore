#include "test_common.h"
#include "atomic_ops.h"
#include "except.h"
#include <pthread.h>
#include <stdint.h>

static void test_init_and_load(void) {
    printf("  Testing AtomicU64_init/load...\n");

    AtomicU64 a;
    AtomicU64_init(&a, 0);
    TEST_ASSERT_EQ(AtomicU64_load(&a), 0, "Freshly initialized atomic should load 0");

    AtomicU64_init(&a, 12345);
    TEST_ASSERT_EQ(AtomicU64_load(&a), 12345, "Atomic should load the value it was initialized with");
}

static void test_fetch_add_returns_previous(void) {
    printf("  Testing AtomicU64_fetch_add returns previous value and updates state...\n");

    AtomicU64 a;
    AtomicU64_init(&a, 10);

    uint64_t prev = AtomicU64_fetch_add(&a, 5);
    TEST_ASSERT_EQ(prev, 10, "fetch_add should return the value before the add");
    TEST_ASSERT_EQ(AtomicU64_load(&a), 15, "fetch_add should update the stored value");

    prev = AtomicU64_fetch_add(&a, 0);
    TEST_ASSERT_EQ(prev, 15, "fetch_add with 0 should return current value unchanged");
    TEST_ASSERT_EQ(AtomicU64_load(&a), 15, "fetch_add with 0 should not change the stored value");
}

static void test_fetch_add_sequence(void) {
    printf("  Testing sequential AtomicU64_fetch_add accumulation...\n");

    AtomicU64 a;
    AtomicU64_init(&a, 0);

    for (int i = 1; i <= 100; i++) {
        AtomicU64_fetch_add(&a, (uint64_t)i);
    }
    /* final value should be 1+2+...+100 = 5050 */
    TEST_ASSERT_EQ(AtomicU64_load(&a), 5050, "Sum of sequential adds 1..100 should be 5050");
}

#define NUM_THREADS 8
#define ADDS_PER_THREAD 50000

typedef struct {
    AtomicU64 *counter;
} IncArgs;

static void *increment_worker(void *arg) {
    IncArgs *args = (IncArgs *)arg;
    for (int i = 0; i < ADDS_PER_THREAD; i++) {
        AtomicU64_fetch_add(args->counter, 1);
    }
    return NULL;
}

static void test_concurrent_fetch_add(void) {
    printf("  Testing AtomicU64_fetch_add correctness under concurrent increment (%d threads)...\n",
           NUM_THREADS);

    AtomicU64 counter;
    AtomicU64_init(&counter, 0);

    pthread_t threads[NUM_THREADS];
    IncArgs args = { &counter };

    for (int i = 0; i < NUM_THREADS; i++) {
        int rc = pthread_create(&threads[i], NULL, increment_worker, &args);
        TEST_ASSERT_EQ(rc, 0, "pthread_create should succeed");
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    uint64_t expected = (uint64_t)NUM_THREADS * ADDS_PER_THREAD;
    TEST_ASSERT_EQ(AtomicU64_load(&counter), expected,
        "Concurrent atomic fetch_add should not lose any increments");
}

bool test_atomic(void) {
    printf("\n=== Atomic Ops Tests ===\n");

    bool result = true;
    TRY
        test_init_and_load();
        test_fetch_add_returns_previous();
        test_fetch_add_sequence();
        test_concurrent_fetch_add();
    EXCEPT(Test_Failed)
        result = false;
    END_TRY;

    return result;
}
