# gramarye-libcore

Shared Library Repo based off of [C Interfaces and Implementations](https://github.com/drh/cii).

Contains extentions like hash algorithims for different datatypes. Putting it in my own repo to reference the project in other projects.

## Features

- Hash tables (Table)
- Sets
- Lists, Arrays, Stacks
- Memory arenas
- String utilities
- Hash functions

## Usage

### Via FetchContent (CMake)

```
include(FetchContent)
FetchContent_Declare(
    gramarye
    GIT_REPOSITORY "https://github.com/yourusername/gramarye-libcore.git"
    GIT_TAG "v1.0.0"
)
FetchContent_MakeAvailable(libcore)
```

