#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>

// size_t alternative, because more than 4GB is unlikely to be needed
typedef uint32_t count_t;

static inline
size_t round_to_alignment(size_t const pointer, size_t const alignment) {
    return pointer + (alignment - (pointer%alignment));
}

typedef struct {
    void** ptr;
    size_t size;
} AllocRequest;

// allocates memory for all requests in one chunk
// element after last should be zero-init'd
static inline
int malloc_many(AllocRequest* const requests) {
    size_t total_memory_requested = 0;
    
    for (size_t i = 0; requests[i].ptr; i++)
        total_memory_requested += requests[i].size;

    size_t offset = 0;  // could be inilned, but name is better
    char* base_ptr = malloc(total_memory_requested);
    if (base_ptr == NULL)
        return 0;

    for (size_t i = 0; requests[i].ptr; i++) {
        *(requests[i].ptr) = base_ptr + offset;
        offset += requests[i].size;
    }
    return total_memory_requested;
}
typedef struct {
    void* data;
    count_t capacity;
    count_t used;
} StackArena;

static inline
StackArena stack_arena_generate(void* const buffer, size_t const capacity) {
    StackArena arena = {0};
    arena.capacity = capacity;
    arena.data = buffer;
    return arena;
}

void* stack_arena_alloc(StackArena* const arena, size_t const size, size_t const alignment) {
    // .data is not assumed to be aligned, so math is done on the pointer, not offset
    if (arena->used >= arena->capacity)
        return NULL;
    size_t const current_end_ptr = (size_t)arena->data + (size_t)arena->used;
    size_t const new_beginning_offset = round_to_alignment(current_end_ptr, alignment) - ((size_t)arena->data);
    arena->used = new_beginning_offset + size;
    return (uint8_t*)arena->data + new_beginning_offset;
}

static inline
void stack_arena_empty(StackArena* const arena) {
    arena->used = 0;
}

typedef struct {
    void* data;            // first few bytes are a free list in the form of a bit set
    count_t* free_list;
    count_t object_size;  // size of one object
    count_t capacity;   // number of objects that fit in the arena
    count_t count;   // number of objects alive
    count_t free_list_count;
} ObjectArena;

/**
 * free list could be a bitset, but then the runtime characteristics are worse, albeit using less memory in the worst case.
 * if .data is 0, error occured.
 */
static inline
ObjectArena object_arena_generate(size_t const object_size, size_t const object_count, StackArena* const arena) {
    ObjectArena output = {0};
    output.capacity = object_count;
    output.object_size = object_size;
    output.free_list = stack_arena_alloc(arena, output.capacity*sizeof(count_t), sizeof(count_t));
    output.data = stack_arena_alloc(arena, output.capacity*object_size, object_size);

    return output;
}
static inline
ObjectArena object_arena_generate_malloc(size_t const object_size, size_t const object_count) {
    ObjectArena arena = {0};
    arena.capacity = object_count;
    arena.object_size = object_size;
    AllocRequest req[] = {
        { .ptr = (void**)&arena.free_list, .size=arena.capacity*sizeof(count_t) },
        { .ptr = (void**)&arena.data,      .size=arena.capacity*object_size     },
        { 0 }
    };
    malloc_many(req);

    return arena;
}

static inline
void* object_arena_alloc(ObjectArena* const arena) {
    if (arena->free_list_count)
        return (uint8_t*)arena->data + arena->free_list[--arena->free_list_count];

    if (arena->count != arena->capacity)
        return (uint8_t*)arena->data + arena->object_size * (arena->count++);
    
    return NULL;
}

static inline
void object_arena_free(ObjectArena* const arena, void const* const object) {
    uint8_t const* const base = (uint8_t*)arena->data;
    uint8_t const* const elem = (uint8_t*)object;
    count_t const index = (elem - base)/arena->object_size;
    if (index == arena->count-1)
        arena->count--;
    else
        arena->free_list[arena->free_list_count++] = index;
}

static inline
void object_arena_empty(ObjectArena* const arena) {
    arena->count = 0;
    arena->free_list_count = 0;
}

// automatically growing unordered collection 
typedef struct {
    ObjectArena arena;
    void**   ptrs;
} Set;

/**
 * consumes n*(8+m) bytes of memory minimum, n*(9+m) max
 * n .. capacity
 * m .. object_size
 */
static inline
Set set_generate(size_t const object_size, size_t const capacity, StackArena* const arena) {
    Set set = {0};
    set.ptrs = stack_arena_alloc(arena, capacity*sizeof(void*), sizeof(void*));
    set.arena = object_arena_generate(object_size, capacity, arena);
    return set;
}

static inline
Set set_generate_malloc(size_t const capacity, size_t const object_size) {
    Set set = {0};
    set.ptrs = malloc(capacity*sizeof(void*));
    set.arena = object_arena_generate_malloc(object_size, capacity);
    set.arena.capacity = capacity;
    return set;
}

static inline
int set_add(Set* const set, void const* const object) {
    if (set->arena.count >= set->arena.capacity)
        return -1;

    // I introduced a memory bug here before
    // lesson: everything inside of set->arena is managed by object_arena_* functions
    // no touchy
    void* const new_storage = object_arena_alloc(&set->arena);
    set->ptrs[set->arena.count-1] = new_storage;  // assign to last entry
    memcpy(new_storage, object, set->arena.object_size);
    return 0;
}

static inline
void set_remove(Set* const set, size_t const i) {
    if (i <= set->arena.capacity)
        return;
    void const* const removed_element = set->ptrs[i];
    set->ptrs[i] = set->ptrs[set->arena.count-1];
    object_arena_free(&set->arena, removed_element);
}

void* set_at(Set const* const set, size_t const i) {
    if (i >= set->arena.capacity)
        return NULL;
    return set->ptrs[i];
}

void set_empty(Set* const set) {
    object_arena_empty(&set->arena);
    set->arena.count = 0;
}

#define set_foreach(set, i) for(size_t i=0; i<(set).arena.count; i++)
