#ifndef GRAMARYE_SYNC_H
#define GRAMARYE_SYNC_H

#if defined(_WIN32)
#include <windows.h>
#else
#include <pthread.h>
#endif

typedef struct SyncMutex {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    int unused;
#elif defined(_WIN32)
    CRITICAL_SECTION native;
#else
    pthread_mutex_t native;
#endif
} SyncMutex;

typedef struct SyncRWLock {
#if defined(GRAMARYE_SYNC_SINGLE_THREADED)
    int unused;
#elif defined(_WIN32)
    SRWLOCK native;
#else
    pthread_rwlock_t native;
#endif
} SyncRWLock;

void SyncMutex_init(SyncMutex* mutex);
void SyncMutex_destroy(SyncMutex* mutex);
void SyncMutex_lock(SyncMutex* mutex);
void SyncMutex_unlock(SyncMutex* mutex);

void SyncRWLock_init(SyncRWLock* lock);
void SyncRWLock_destroy(SyncRWLock* lock);
void SyncRWLock_read_lock(SyncRWLock* lock);
void SyncRWLock_read_unlock(SyncRWLock* lock);
void SyncRWLock_write_lock(SyncRWLock* lock);
void SyncRWLock_write_unlock(SyncRWLock* lock);

#endif


