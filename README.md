# Anasi: Arena library written in C

I'm pretty sure anyone looking at this README for help doesn't have the patience for flowery language, hence most of it is rather terse.

## usage:
1. configuration:
before the include, add
`#define ANASI_STRING` for string utilities
`#define ANASI_MALLOC` for malloc convenience utilities

2. include
Add the following to the top of your implementation file:
`#include "path/to/anasi/include/mem.h"`

## API:
Each structure has it's own set of functions for init/creation and mutation prefixed by their name.
Every structure has a `generate` function for creating a new structure and a `empty` for emptying out the structure.
Most have a `free` function for freeing invidivual elements of a arena.

There are no destructors (`malloc_`* are a special case).
There are no getters and setters.
There are no implementation files in the base library.

## list of structures implemented:
- `stack_arena` or bump allocator/arena, or simply arena
  - The base and common api used for the other structures or on it's own.
- `object_arena`, free list, bucket allocator/arena
- `set`, an unordered, discontinous, iterable/index-addressable `object_arena` with support for iterating over every element and adding and removing elements by pointers
- `string_arena` (Anasi `string.h`)-- a special purpose storage for storing arbitrary length strings in an `object_arena`

## special structures:
- `LINKED_LIST_ELEMENT_OF(TYPE)` & `list_foreach(first, e)`
  - macros for defining an element of a linked list and iterating over a linked list starting at a given node, respectively
- `ARRAY`: a type-safe, contiguous array allocator macro using a `stack_arena` as the backing store


## additional functionality:
- `SIZED_STRING` macro for turning a C string into a sized `String` (fat pointer to string)
- `string_append` for concatenating sized `String`s
- `string_build_in_stack_arena` concatenates strings from a null-terminated array into a single sized `String` backed by a `stack_arena`
- `string_compare` for comparing sized `String`s
