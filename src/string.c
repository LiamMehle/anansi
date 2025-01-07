#include <string.h>
#include <stdint.h>
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