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

typedef LINKED_LIST_ELEMENT_OF(Entry) EntryElement;
/**
 * I'd make use of an object/freelist arena, but then I'd have to split the
 * master_arena between strings and Entry elements
 */
EntryElement* enumerate_fs_path(String base_path, StackArena* const master_arena) {
	size_t const max_expected_entry_count = 512;

	String search_path = string_build_in_stack_arena(master_arena, (String[]){
		base_path,
		SIZED_STRING("\\*.*"),
		{ 0 }
	});

	WIN32_FIND_DATA fd; 
	HANDLE directory_handle = FindFirstFile(search_path.str, &fd);

	// handle built, the string can be freed (which amounts to an integer write)
	stack_arena_empty(master_arena);

	EntryElement* previous_element = NULL;
	EntryElement* first_element = NULL;

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

			EntryElement* entry = STACK_ARENA_ALLOC(EntryElement, master_arena);
			*entry = (EntryElement){
				.item = {
					.path = path.str,
					.type = fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? dir : file
				},
				.next = NULL
			};

			// branches bad, yes, but these are very predictable
			if (!first_element)
				first_element = entry;
			if (previous_element)
				previous_element->next = entry;

			previous_element = entry;
		} while (FindNextFile(directory_handle, &fd)); 
		FindClose(directory_handle); 
	}
	return first_element;
}

int main() {
	// configuration
	size_t const malloc_size  = 16000;
	size_t const scratch_size = malloc_size;

	// our entire malloc allowance
	void* true_dynamic_memory = malloc(malloc_size);

	// arena setup to organize malloc use
	StackArena arena = stack_arena_generate(true_dynamic_memory, scratch_size);

	EntryElement const* const first_entry = enumerate_fs_path(SIZED_STRING("."), &arena);

	size_t const scratch_consumed_memory = arena.used;
	printf("\nused %zu bytes", scratch_consumed_memory);

	EntryElement const* entry = first_entry;
	do {
		if (entry->item.path)
			printf("%s\n", entry->item.path);
		else
			puts("<ERROR>");
	} while((entry = entry->next));

	puts("done");
	stack_arena_empty(&arena);
	return 0;
}
