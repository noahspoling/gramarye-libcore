#include "sync.h"

void SyncMutex_init(SyncMutex* mutex) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)mutex;
#elif defined(_WIN32)
    InitializeCriticalSection(&mutex->native);
#else
    pthread_mutex_init(&mutex->native, NULL);
#endif
}

void SyncMutex_destroy(SyncMutex* mutex) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)mutex;
#elif defined(_WIN32)
    DeleteCriticalSection(&mutex->native);
#else
    pthread_mutex_destroy(&mutex->native);
#endif
}

void SyncMutex_lock(SyncMutex* mutex) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)mutex;
#elif defined(_WIN32)
    EnterCriticalSection(&mutex->native);
#else
    pthread_mutex_lock(&mutex->native);
#endif
}

void SyncMutex_unlock(SyncMutex* mutex) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)mutex;
#elif defined(_WIN32)
    LeaveCriticalSection(&mutex->native);
#else
    pthread_mutex_unlock(&mutex->native);
#endif
}

void SyncRWLock_init(SyncRWLock* lock) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)lock;
#elif defined(_WIN32)
    InitializeSRWLock(&lock->native);
#else
    pthread_rwlock_init(&lock->native, NULL);
#endif
}

void SyncRWLock_destroy(SyncRWLock* lock) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)lock;
#elif defined(_WIN32)
    (void)lock;
#else
    pthread_rwlock_destroy(&lock->native);
#endif
}

void SyncRWLock_read_lock(SyncRWLock* lock) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)lock;
#elif defined(_WIN32)
    AcquireSRWLockShared(&lock->native);
#else
    pthread_rwlock_rdlock(&lock->native);
#endif
}

void SyncRWLock_read_unlock(SyncRWLock* lock) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)lock;
#elif defined(_WIN32)
    ReleaseSRWLockShared(&lock->native);
#else
    pthread_rwlock_unlock(&lock->native);
#endif
}

void SyncRWLock_write_lock(SyncRWLock* lock) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)lock;
#elif defined(_WIN32)
    AcquireSRWLockExclusive(&lock->native);
#else
    pthread_rwlock_wrlock(&lock->native);
#endif
}

void SyncRWLock_write_unlock(SyncRWLock* lock) {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    (void)lock;
#elif defined(_WIN32)
    ReleaseSRWLockExclusive(&lock->native);
#else
    pthread_rwlock_unlock(&lock->native);
#endif
}


