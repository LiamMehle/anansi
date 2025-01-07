#include <stdio.h>
#include <stdint.h>
#include <windows.h>

// yes, it's an implementation file
// these functions are gonna get inlined anyway
#include "mem.c"
#include "string.c"

enum EntryType {
    file,
    dir
};
typedef struct {
    uint16_t path_offset;  // compressed pointer stored as offset into arena
    uint8_t type;
} Entry;

void enumerate_fs_path(String base_path, Set* const files, StackArena* const arena) {
    String search_path = string_build_in_stack_arena(arena, (String[]){
        base_path,
        SIZED_STRING("\\*.*"),
        { 0 }
    });


    WIN32_FIND_DATA fd; 
    HANDLE directory_handle = FindFirstFile(search_path.str, &fd); 
    if(directory_handle != INVALID_HANDLE_VALUE) {
        do {
            // read all (real) files in current folder
            // , delete '!' read other 2 default folder . and ..
            if(fd.cFileName[0] == '.' && (fd.cFileName[1] == '\0' || fd.cFileName[1] == '.'))
                continue;

            String path = string_build_in_stack_arena(arena, (String[]){
                base_path,
                SIZED_STRING("\\"),
                SIZED_STRING(fd.cFileName),
                { 0 }
            });

            Entry entry = {
                .path_offset = (size_t)path.str - (size_t)arena->data,
                .type = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dir : file
            };
            set_add(files, &entry);
        }while(FindNextFile(directory_handle, &fd)); 
        FindClose(directory_handle); 
    } 
}

int main() {
    Set entry_set = set_generate_malloc(512, sizeof(Entry));
    size_t const scratch_size = 1024;
    StackArena arena = stack_arena_generate(malloc(scratch_size), scratch_size);
    enumerate_fs_path(SIZED_STRING("."), &entry_set, &arena);

    size_t const entry_set_consumed_memory = entry_set.count*(entry_set.arena.object_size+sizeof(void*));
    size_t const scratch_consumed_memory = arena.used;
    printf("\nused %zu bytes -- %zu scratch, %zu set\n",
        entry_set_consumed_memory+scratch_consumed_memory,
        scratch_consumed_memory,
        entry_set_consumed_memory);
    set_foreach(entry_set, i) {
        Entry* entry = set_at(&entry_set, i);
        if (entry)
            puts((char*)arena.data + entry->path_offset);
        else
            puts("<ERROR>");
    }

    puts("done");
    stack_arena_empty(&arena);
    return 0;
}
// .\*.*.\CMakeCache.txt.\CMakeLists.txt.\cmake_install.cmake.\compressor.exe.\Makefile