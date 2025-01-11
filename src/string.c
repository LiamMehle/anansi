#include <minwindef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "mem.c"

typedef struct {
    char* str;
    count_t len;
    count_t capacity;  // 0 has the semantics of "immutable" or "can hold `len` chars"
} String;

static inline
String string_append(String string, String other) {
    if (string.capacity - string.len >= other.len-1) {
        memcpy(string.str + string.len, other.str, other.len);
        string.len += other.len;
        string.str[string.len] = 0;
    }
    return string;
}

static inline
String string_build_in_stack_arena(StackArena* const arena, String* strings) {
    size_t total_len = 0;
    for (int i=0; strings[i].str; i++)
        total_len += strings[i].len;

    String string = {
        .str = stack_arena_alloc(arena, total_len, 1),
        .len = 0,
        .capacity = total_len
    };

    if (!string.str) {
        String s = { 0 };
        return s;
    }

    for (int i=0; strings[i].str; i++) {
        string = string_append(string, strings[i]);
    }

    return string;
}

#define SIZED_STRING(STR) (String){ .str=STR, .len=strlen(STR), .capacity=0 }

#define fragment_size (64-sizeof(count_t))
struct StringSegment {
    struct StringSegment* next;
    char fragment[fragment_size];  // last byte determines spare capacity
};

typedef struct {
    struct StringSegment* first_segment;
    size_t         total_len;
} FragmentedStringHandle;

typedef ObjectArena StringArena;

StringArena string_arena_generate(StackArena* const arena, size_t const segment_count) {
    return object_arena_generate(sizeof(struct StringSegment), segment_count, arena);
}

FragmentedStringHandle string_arena_store(StringArena* const arena, String remaining_string) {
    struct StringSegment* previous_segment = object_arena_alloc(arena);
    FragmentedStringHandle output = {
        .first_segment = previous_segment,
        .total_len = remaining_string.len
    };
    memcpy(&previous_segment->fragment, remaining_string.str, min(remaining_string.len, fragment_size));
    remaining_string.str += min(remaining_string.len, fragment_size);
    remaining_string.len -= min(remaining_string.len, fragment_size);
    while (remaining_string.len > 0) {
        struct StringSegment* segment = object_arena_alloc(arena);

        memcpy(&segment->fragment, remaining_string.str, min(remaining_string.len, fragment_size));
        remaining_string.str += min(remaining_string.len, fragment_size);
        remaining_string.len -= min(remaining_string.len, fragment_size);

        previous_segment->next = segment;
        previous_segment = segment;
    }
    return output;
}

String string_arena_load(StringArena* const arena, FragmentedStringHandle const string_handle, StackArena* const stack_arena) {
    String output = {
        .str      = stack_arena_alloc(stack_arena, string_handle.total_len, string_handle.total_len),
        .len      = string_handle.total_len,
        .capacity = string_handle.total_len,
    };
    struct StringSegment* segment = string_handle.first_segment;
    char* ptr = output.str;
    size_t remaining_len = output.len;
    while (TRUE) {
        memcpy(ptr, &segment->fragment, min(remaining_len, fragment_size));
        remaining_len -= fragment_size;
        if (segment->next)
            segment = segment->next;
        else
            break;
    }
    return output;
}

void string_arena_free(StringArena* const arena, FragmentedStringHandle const string_handle) {
    struct StringSegment* segment = string_handle.first_segment;
    while (TRUE) {
        object_arena_free(arena, segment);
        if (segment->next)
            segment = segment->next;
        else
            break;
    }
}