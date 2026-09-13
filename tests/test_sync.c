#include "test_common.h"
#include "sync.h"
#include "except.h"
#include <pthread.h>
#include <unistd.h>

static void test_mutex_basic_lock_unlock(void) {
    printf("  Testing basic mutex init/lock/unlock/destroy...\n");

    SyncMutex mutex;
    SyncMutex_init(&mutex);
    SyncMutex_lock(&mutex);
    SyncMutex_unlock(&mutex);
    SyncMutex_lock(&mutex);
    SyncMutex_unlock(&mutex);
    SyncMutex_destroy(&mutex);

    TEST_ASSERT_TRUE(1, "Mutex lifecycle should complete without deadlock or crash");
}

typedef struct {
    SyncMutex *mutex;
    long *counter;
    int iterations;
} MutexWorkerArgs;

static void *mutex_worker(void *arg) {
    MutexWorkerArgs *args = (MutexWorkerArgs *)arg;
    for (int i = 0; i < args->iterations; i++) {
        SyncMutex_lock(args->mutex);
        long v = *args->counter;
        /* Encourage interleaving if the mutex were not actually exclusive. */
        v = v + 1;
        *args->counter = v;
        SyncMutex_unlock(args->mutex);
    }
    return NULL;
}

static void test_mutex_mutual_exclusion(void) {
    printf("  Testing mutex mutual exclusion under contention (4 threads)...\n");

    const int NUM_THREADS = 4;
    const int ITERATIONS = 20000;

    SyncMutex mutex;
    SyncMutex_init(&mutex);

    long counter = 0;
    pthread_t threads[4];
    MutexWorkerArgs args[4];

    for (int i = 0; i < NUM_THREADS; i++) {
        args[i].mutex = &mutex;
        args[i].counter = &counter;
        args[i].iterations = ITERATIONS;
        int rc = pthread_create(&threads[i], NULL, mutex_worker, &args[i]);
        TEST_ASSERT_EQ(rc, 0, "pthread_create should succeed");
    }
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    TEST_ASSERT_EQ(counter, (long)NUM_THREADS * ITERATIONS,
        "Counter incremented under mutex protection should equal total iterations exactly");

    SyncMutex_destroy(&mutex);
}

static void test_rwlock_basic(void) {
    printf("  Testing basic rwlock init/read/write/destroy...\n");

    SyncRWLock lock;
    SyncRWLock_init(&lock);

    SyncRWLock_read_lock(&lock);
    SyncRWLock_read_unlock(&lock);

    SyncRWLock_write_lock(&lock);
    SyncRWLock_write_unlock(&lock);

    SyncRWLock_destroy(&lock);

    TEST_ASSERT_TRUE(1, "RWLock lifecycle should complete without deadlock or crash");
}

typedef struct {
    SyncRWLock *lock;
    long *shared_value;
    int iterations;
    int is_writer;
    long *writes_seen_consistent; /* readers set to 0 if they observe a torn read */
} RWWorkerArgs;

static void *rwlock_writer(void *arg) {
    RWWorkerArgs *args = (RWWorkerArgs *)arg;
    for (int i = 0; i < args->iterations; i++) {
        SyncRWLock_write_lock(args->lock);
        /* Write a value where both halves (via two fields simulated with one
           long here) must be observed consistently: use a value that is
           always a multiple of 2 while being written, and odd momentarily
           is not possible since this is a single word -- instead verify no
           crash/corruption of the counter itself. */
        long v = *args->shared_value;
        *args->shared_value = v + 1;
        SyncRWLock_write_unlock(args->lock);
    }
    return NULL;
}

static void *rwlock_reader(void *arg) {
    RWWorkerArgs *args = (RWWorkerArgs *)arg;
    long last = -1;
    for (int i = 0; i < args->iterations; i++) {
        SyncRWLock_read_lock(args->lock);
        long v = *args->shared_value;
        SyncRWLock_read_unlock(args->lock);
        if (v < last) {
            *args->writes_seen_consistent = 0;
        }
        last = v;
    }
    return NULL;
}

static void test_rwlock_readers_writers(void) {
    printf("  Testing rwlock with concurrent readers and a writer...\n");

    SyncRWLock lock;
    SyncRWLock_init(&lock);

    long shared_value = 0;
    long consistent = 1;

    const int NUM_READERS = 3;
    pthread_t writer_thread;
    pthread_t reader_threads[3];

    RWWorkerArgs writer_args = { &lock, &shared_value, 5000, 1, &consistent };
    RWWorkerArgs reader_args[3];

    pthread_create(&writer_thread, NULL, rwlock_writer, &writer_args);
    for (int i = 0; i < NUM_READERS; i++) {
        reader_args[i] = (RWWorkerArgs){ &lock, &shared_value, 5000, 0, &consistent };
        pthread_create(&reader_threads[i], NULL, rwlock_reader, &reader_args[i]);
    }

    pthread_join(writer_thread, NULL);
    for (int i = 0; i < NUM_READERS; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    TEST_ASSERT_EQ(shared_value, 5000, "Writer should have incremented shared value exactly iterations times");
    TEST_ASSERT_EQ(consistent, 1, "Readers should never observe the counter decrease (monotonic under lock)");

    SyncRWLock_destroy(&lock);
}

bool test_sync(void) {
    printf("\n=== Sync Tests ===\n");

    bool result = true;
    TRY
        test_mutex_basic_lock_unlock();
        test_mutex_mutual_exclusion();
        test_rwlock_basic();
        test_rwlock_readers_writers();
    EXCEPT(Test_Failed)
        result = false;
    END_TRY;

    return result;
}
