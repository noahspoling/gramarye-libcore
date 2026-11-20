// Windows implementation of the CII Thread/Sem interfaces.
//
// Same semantics as src/platform/unix/thread.c (the pthread backend), so code
// and tests behave identically on both platforms:
//   - each thread finds its own Thread_T through thread-local storage, set by
//     the thread itself before it runs anything (no lookup table, so no race
//     between a new thread starting and its creator registering it);
//   - a finished thread's record and exit code survive until it is joined;
//   - Thread_init may be called more than once (each call installs a fresh
//     handle for the calling thread);
//   - Thread_join(NULL) returns 0 immediately;
//   - Thread_alert interrupts Thread_join and Sem_wait, which then raise
//     Thread_Alerted in the waiting thread.
//
// Waits use WaitForMultipleObjects on {the thing being waited for, the
// waiter's own alert event}, so an alert can never be missed between checking
// the flag and starting to wait.

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <process.h>

#include "assert.h"
#include "except.h"
#include "mem.h"
#include "thread.h"
#include "sem.h"

static char rcsid[] = "$Id$";

#define T Thread_T

struct T {
    T        handle;      // self pointer; NULL once joined
    HANDLE   thread;      // OS thread handle; NULL for a Thread_init (root) handle
    HANDLE   alert;       // auto-reset event, set by Thread_alert
    volatile LONG alerted;// 1 if this thread has been alerted
    int      code;        // exit code, valid once `thread` is signaled
    int      joined;      // only one joiner (matches the pthread backend)
    int    (*apply)(void *);
    void    *args;
};

const Except_T Thread_Alerted = { "Thread alerted" };
const Except_T Thread_Failed  = { "Thread creation failed" };

/* ---- Thread-local "self" ---------------------------------------------------- */

static DWORD     self_index = TLS_OUT_OF_INDEXES;
static INIT_ONCE self_once  = INIT_ONCE_STATIC_INIT;

static BOOL CALLBACK make_self_index(PINIT_ONCE once, PVOID param, PVOID *ctx) {
    (void)once; (void)param; (void)ctx;
    self_index = TlsAlloc();
    return self_index != TLS_OUT_OF_INDEXES;
}

static T self_get(void) {
    if (!InitOnceExecuteOnce(&self_once, make_self_index, NULL, NULL)) return NULL;
    return (T)TlsGetValue(self_index);
}

static void self_set(T t) {
    if (InitOnceExecuteOnce(&self_once, make_self_index, NULL, NULL)) {
        TlsSetValue(self_index, t);
    }
}

/* ---- Alerts ---------------------------------------------------------------- */

static void testalert(T current) {
    if (current && InterlockedExchange(&current->alerted, 0)) {
        RAISE(Thread_Alerted);
    }
}

// Waits for `object` to be signaled while staying interruptible by an alert
// on the calling thread. Returns normally once `object` is signaled.
static void wait_alertable(T current, HANDLE object) {
    HANDLE handles[2] = { object, current->alert };
    for (;;) {
        DWORD r = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
        assert(r != WAIT_FAILED);
        if (r == WAIT_OBJECT_0) return;
        testalert(current);   // raises if alerted; a stale event just loops
    }
}

static T new_handle(size_t extra) {
    T t;
    TRY
        t = ALLOC((sizeof(*t) + extra + 15) & ~(size_t)15);
        memset(t, 0, sizeof(*t));
    EXCEPT(Mem_Failed)
        RAISE(Thread_Failed);
    END_TRY;
    t->alert = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (t->alert == NULL) {
        FREE(t);
        RAISE(Thread_Failed);
    }
    t->handle = t;
    return t;
}

/* ---- Threads ----------------------------------------------------------------- */

static unsigned __stdcall start(void *p) {
    T t = (T)p;
    self_set(t);
    // CII exceptions keep a per-thread handler stack in TLS (except.c).
    if (Except_index == -1)
        Except_init();
    TlsSetValue(Except_index, NULL);

    t->code = t->apply ? t->apply(t->args) : 0;
    return (unsigned)t->code;
}

int Thread_init(int preempt, ...) {
    (void)preempt;   // preemption is the OS scheduler's job in this backend
    T root;
    TRY
        root = new_handle(0);
    EXCEPT(Thread_Failed)
        return -1;
    END_TRY;
    self_set(root);
    return 1;
}

T Thread_self(void) {
    T t = self_get();
    assert(t);
    return t;
}

void Thread_pause(void) {
    SwitchToThread();
}

void Thread_alert(T t) {
    assert(t && t->handle == t);
    InterlockedExchange(&t->alerted, 1);
    SetEvent(t->alert);
}

int Thread_join(T t) {
    T current = Thread_self();
    assert(t != current);
    testalert(current);

    if (t == NULL) return 0;   // no registry of all threads (matches pthread backend)
    if (t->handle != t || t->joined || t->thread == NULL) return -1;
    t->joined = 1;

    wait_alertable(current, t->thread);

    int code = t->code;        // written by the thread before its handle signaled
    CloseHandle(t->thread);
    CloseHandle(t->alert);
    t->handle = NULL;
    FREE(t);

    testalert(current);
    return code;
}

T Thread_new(int apply(void *), void *args, int nbytes, ...) {
    (void)Thread_self();       // the creator must have an identity (Thread_init)
    assert(apply);
    assert((args && nbytes >= 0) || args == NULL);
    if (args == NULL) nbytes = 0;

    T t = new_handle((size_t)nbytes);
    t->apply = apply;
    if (nbytes > 0) {
        t->args = (void *)(t + 1);
        memcpy(t->args, args, (size_t)nbytes);
    } else {
        t->args = args;
    }

    // Fully initialized before the thread exists; the thread sets its own
    // TLS self in start(), so there is nothing left to race on.
    t->thread = (HANDLE)_beginthreadex(NULL, 0, start, t, 0, NULL);
    if (t->thread == NULL) {
        CloseHandle(t->alert);
        FREE(t);
        RAISE(Thread_Failed);
    }
    return t;
}

void Thread_exit(int code) {
    T t = Thread_self();
    t->code = code;
    _endthreadex((unsigned)code);
}

#undef T

/* ---- Semaphores (Sem_T) on a native Win32 semaphore ------------------------- */

#define T Sem_T

T *Sem_new(int count) {
    T *s;
    NEW(s);
    Sem_init(s, count);
    return s;
}

// s->count is the initial count only; the live count is inside the native
// semaphore (Windows has no race-free way to read it).
void Sem_init(T *s, int count) {
    assert(s);
    assert(count >= 0);
    s->count = count;
    s->queue = CreateSemaphore(NULL, count, INT_MAX, NULL);
    assert(s->queue);
}

void Sem_wait(T *s) {
    assert(s && s->queue);
    Thread_T current = Thread_self();
    testalert(current);
    wait_alertable(current, (HANDLE)s->queue);
    // Alerted while acquiring: hand the unit back so it isn't lost, then raise.
    if (InterlockedExchange(&current->alerted, 0)) {
        ReleaseSemaphore((HANDLE)s->queue, 1, NULL);
        RAISE(Thread_Alerted);
    }
}

void Sem_signal(T *s) {
    assert(s && s->queue);
    BOOL ok = ReleaseSemaphore((HANDLE)s->queue, 1, NULL);
    assert(ok);
    (void)ok;
}

#undef T
