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
	char* path;
	uint8_t type;
} Entry;

Set enumerate_fs_path(String base_path, StackArena* const master_arena) {
	size_t const max_expected_entry_count = 32;
	Set entries = set_generate(sizeof(Entry), max_expected_entry_count, master_arena);

	String search_path = string_build_in_stack_arena(master_arena, (String[]){
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

			String const path = string_build_in_stack_arena(master_arena, (String[]){
				base_path,
				SIZED_STRING("\\"),
				SIZED_STRING(fd.cFileName),
				{ 0 }
			});
			
			if (!path.str)  // string won't fit in memory
				continue;

			Entry entry = {
				.path = path.str,
				.type = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dir : file
			};
			if (set_add(&entries, &entry))
				break;
		} while (FindNextFile(directory_handle, &fd)); 
		FindClose(directory_handle); 
	}
	return entries;
}

int main() {
	// configuration
	size_t const malloc_size  = 16000;
	size_t const scratch_size = malloc_size;

	// our entire malloc allowance
	void* true_dynamic_memory = malloc(malloc_size);

	// arena setup to organize malloc use
	StackArena arena = stack_arena_generate(true_dynamic_memory, scratch_size);

	Set entry_set = enumerate_fs_path(SIZED_STRING("."), &arena);

	size_t const entry_set_consumed_memory = entry_set.arena.count*(entry_set.arena.object_size+sizeof(void*));
	size_t const scratch_consumed_memory = arena.used;
	printf("entry count: %u\n", entry_set.arena.count);
	printf("\nused %zu bytes -- %zu scratch, %zu set\n",
		entry_set_consumed_memory+scratch_consumed_memory,
		scratch_consumed_memory,
		entry_set_consumed_memory);
	set_foreach(entry_set, i) {
		Entry* entry = set_at(&entry_set, i);
		if (entry)
			printf("%zu: %s\n", i, entry->path);
		else
			puts("<ERROR>");
	}

	puts("done");
	stack_arena_empty(&arena);
	return 0;
}
