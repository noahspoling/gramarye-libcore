# gramarye-libcore

Shared Library Repo based off of [C Interfaces and Implementations](https://github.com/drh/cii).

Contains extensions like hash algorithms for different datatypes. Putting it in my own repo to reference the project in other projects.

## Overview

gramarye-libcore provides fundamental data structures and utilities used throughout the Gramarye game engine. It's based on the C Interfaces and Implementations library with additional hash functions and utilities.

## Features

### Data Structures

- **Table**: Hash table with customizable comparison and hash functions
- **Set**: Set data structure built on Table
- **List**: Doubly-linked list
- **Array**: Dynamic array with automatic resizing
- **Stack**: Stack data structure
- **Seq**: Sequence (dynamic array with high/low operations)
- **Ring**: Ring buffer
- **Arena**: Memory arena for efficient allocation/deallocation

### Utilities

- **String utilities**: String manipulation and formatting
- **Memory management**: Arena-based and traditional malloc-based allocation
- **Hash functions**: Hash functions for various types:
  - UUID hashing (128-bit UUIDs)
  - IntCoord hashing (2D integer coordinates)
  - Simple string hashing
- **Threading**: Thread and semaphore abstractions (platform-specific implementations)
- **Exception handling**: Exception mechanism for error handling
- **Formatting**: String formatting utilities

### Platform Support

- **Unix/Linux**: pthread-based threading implementation
- **Windows**: Windows-specific threading implementation
- **Cross-platform**: Most utilities work on all platforms

## Usage

### Via FetchContent (CMake)

```cmake
include(FetchContent)
FetchContent_Declare(
    gramarye_libcore
    GIT_REPOSITORY "https://github.com/noahspoling/gramarye-libcore.git"
    GIT_TAG "main"
    GIT_PROGRESS TRUE
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(gramarye_libcore)
```

### Basic Usage

```c
#include "arena.h"
#include "table.h"
#include "hash/uuid_hash.h"

// Create arena
Arena_T arena = Arena_new();

// Create hash table
Table_T table = Table_new(256, int_cmp, int_hash);

// Allocate in arena
void* data = Arena_alloc(arena, sizeof(MyStruct), __FILE__, __LINE__);

// Use table
int key = 42;
Table_put(table, &key, data);
void* value = Table_get(table, &key);
```

## Hash Functions

### UUID Hashing

For 128-bit UUIDs (EntityId in ECS):

```c
#include "hash/uuid_hash.h"

EntityId id = {.high = 1, .low = 2};
unsigned hash = hash_uuid(id);
```

### IntCoord Hashing

For 2D integer coordinates:

```c
#include "hash/int_coord_hash.h"

IntCoord coord = {.x = 10, .y = 20};
unsigned hash = IntCoord_hash(&coord);
int cmp = IntCoord_cmp(&coord1, &coord2);
```

## Threading

Threading support is platform-specific:

- **Unix**: Uses pthreads (`pthread_create`, `pthread_join`, etc.)
- **Windows**: Uses Windows threads

The interface abstracts platform differences:

```c
#include "thread.h"
#include "sem.h"

Thread_T thread = Thread_new(thread_func, userData, stackSize);
Thread_join(thread);

Sem_T sem;
Sem_init(&sem, count);
Sem_wait(&sem);
Sem_signal(&sem);
```

## Dependencies

- No external dependencies (pure C library)
- Platform-specific code for threading (pthreads on Unix, Windows threads on Windows)

## Building

This library is typically built as part of the main game project via CMake FetchContent or as a subdirectory. See the main project's CMakeLists.txt for build configuration.

## Integration

This library is the foundation for all other Gramarye modules. It provides the core data structures and utilities that higher-level systems depend on.

