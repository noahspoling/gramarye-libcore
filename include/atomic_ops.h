#ifndef GRAMARYE_ATOMIC_OPS_H
#define GRAMARYE_ATOMIC_OPS_H

#include <stdint.h>

#if defined(_MSC_VER) || defined(GRAMARYE_ATOMICS_MSVC)
#include <windows.h>
#endif

#if defined(GRAMARYE_ATOMICS_C11)
#include <stdatomic.h>
#elif !defined(GRAMARYE_ATOMICS_SINGLE) && !defined(_MSC_VER) && !defined(GRAMARYE_ATOMICS_BUILTIN) && !defined(GRAMARYE_ATOMICS_MSVC)
#include <stdatomic.h>
#endif

typedef struct AtomicU64 {
#if defined(GRAMARYE_ATOMICS_SINGLE)
    uint64_t value;
#elif defined(_MSC_VER) || defined(GRAMARYE_ATOMICS_MSVC)
    volatile LONG64 value;
#elif defined(GRAMARYE_ATOMICS_BUILTIN)
    volatile uint64_t value;
#else
    _Atomic uint64_t value;
#endif
} AtomicU64;

static inline void AtomicU64_init(AtomicU64* atomicValue, uint64_t value) {
#if defined(GRAMARYE_ATOMICS_SINGLE)
    atomicValue->value = value;
#elif defined(_MSC_VER) || defined(GRAMARYE_ATOMICS_MSVC)
    atomicValue->value = (LONG64)value;
#elif defined(GRAMARYE_ATOMICS_BUILTIN)
    __atomic_store_n(&atomicValue->value, value, __ATOMIC_RELAXED);
#else
    atomic_init(&atomicValue->value, value);
#endif
}

static inline uint64_t AtomicU64_load(const AtomicU64* atomicValue) {
#if defined(GRAMARYE_ATOMICS_SINGLE)
    return atomicValue->value;
#elif defined(_MSC_VER) || defined(GRAMARYE_ATOMICS_MSVC)
    return (uint64_t)InterlockedCompareExchange64((volatile LONG64*)&atomicValue->value, 0, 0);
#elif defined(GRAMARYE_ATOMICS_BUILTIN)
    return __atomic_load_n(&atomicValue->value, __ATOMIC_RELAXED);
#else
    return atomic_load_explicit(&atomicValue->value, memory_order_relaxed);
#endif
}

static inline uint64_t AtomicU64_fetch_add(AtomicU64* atomicValue, uint64_t increment) {
#if defined(GRAMARYE_ATOMICS_SINGLE)
    uint64_t oldValue = atomicValue->value;
    atomicValue->value += increment;
    return oldValue;
#elif defined(_MSC_VER) || defined(GRAMARYE_ATOMICS_MSVC)
    return (uint64_t)(InterlockedAdd64(&atomicValue->value, (LONG64)increment) - (LONG64)increment);
#elif defined(GRAMARYE_ATOMICS_BUILTIN)
    return __atomic_fetch_add(&atomicValue->value, increment, __ATOMIC_RELAXED);
#else
    return atomic_fetch_add_explicit(&atomicValue->value, increment, memory_order_relaxed);
#endif
}

#endif

