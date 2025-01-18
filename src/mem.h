#pragma once
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <stdalign.h>

/**
 * General rules of argument order:
 * (macros only) Types first
 *               most relevant object, especially if in the name of the function
 *               ...other arguments
 * size      (if required) comes after the object it relates to
 * alignment (if required) comes after (or in place of) the size of the object it relates to
 */

// size_t alternative, because more than 4GB is unlikely to be needed
typedef uint32_t count_t;



// ------------------------ STACK ARENA ------------------------
/**
 * The default arena and used for type erasure
 * O(1) allocator (pointer bump)
 * no-op deallocation
 */
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
    char* base_ptr = (char*)malloc(total_memory_requested);
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

static inline
void* stack_arena_alloc(StackArena* const arena, size_t const size, size_t const alignment) {
    // .data is not assumed to be aligned, so math is done on the pointer, not offset
    if (arena->used >= arena->capacity)
        return NULL;
    size_t const current_end_ptr = (size_t)arena->data + (size_t)arena->used;
    size_t const new_beginning_offset = round_to_alignment(current_end_ptr, alignment) - ((size_t)arena->data);
    arena->used = new_beginning_offset + size;
    return (uint8_t*)arena->data + new_beginning_offset;
}
/** same name because it has the same semantics as using the function correctly
 *  and because less typing means people are more likely to use it
 */
#define STACK_ARENA_ALLOC(TYPE, ARENA) (TYPE*)stack_arena_alloc((ARENA), sizeof(TYPE), alignof(TYPE))

static inline
void stack_arena_empty(StackArena* const arena) {
    arena->used = 0;
}



// ------------------------ OBJECT/FREELIST ARENA ------------------------
/**
 * Assumes allocation in fixed-size chunks
 * Is able to avoid fragmentation that would obstruct allocation
 * O(1) allocation, deallocation
 */
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
    output.capacity    = object_count;
    output.object_size = object_size;
    output.free_list   = stack_arena_alloc(arena, output.capacity*sizeof(count_t), alignof(void*));
    output.data        = stack_arena_alloc(arena, output.capacity*object_size, alignof(size_t));

    return output;
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


// ------------------------ SET CONTAINER ------------------------
/**
 * Object/freelist arena with pointer array for efficient traveral of elements in order
 */
typedef struct {
    ObjectArena arena;
    count_t*    offsets;       // contigous `arena.count`-length array of integers to entries in (T[])arena.data
} Set;

/**
 * consumes n*(8+m) bytes of memory minimum, n*(9+m) max
 * n .. capacity
 * m .. object_size
 */
static inline
Set set_generate(size_t const object_size, size_t const capacity, StackArena* const arena) {
    Set set = { 0 };
    set.offsets = stack_arena_alloc(arena, capacity*sizeof(*set.offsets), sizeof(*set.offsets));
    set.arena = object_arena_generate(object_size, capacity, arena);
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
    set->offsets[set->arena.count-1] = ((uint8_t*)new_storage - (uint8_t*)set->arena.data)/set->arena.object_size;  // assign to last entry
    memcpy(new_storage, object, set->arena.object_size);
    return 0;
}

static inline
void* set_at(Set const* const set, size_t const i) {
    if (i >= set->arena.capacity)
        return NULL;
    return ((char*)set->arena.data) + (set->offsets[i]*set->arena.object_size);
}

static inline
void set_remove(Set* const set, size_t const i) {
    if (i <= set->arena.capacity)
        return;
    void const* const removed_element = set_at(set, i);
    set->offsets[i] = set->offsets[set->arena.count-1];
    object_arena_free(&set->arena, removed_element);
}

static inline
void set_empty(Set* const set) {
    object_arena_empty(&set->arena);
    set->arena.count = 0;
}

// for-loop over every element in set
#define set_foreach(set, i) for(size_t i=0; i<(set).arena.count; i++)



// ------------------------ LINKED LIST (element) ------------------------
// defines struct that serves as element of linked list of TYPE
#define LINKED_LIST_ELEMENT_OF(TYPE) \
struct TYPE##Element {               \
    struct TYPE##Element* next;      \
    TYPE item;                       \
}

#define list_foreach(first, e) for(typeof(*first) const* entry = first; entry; entry = entry->next)
