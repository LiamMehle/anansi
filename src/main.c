#include <stdio.h>
#include <stdint.h>
#include <windows.h>

// yes, it's an implementation file
// these functions are gonna get inlined anyway
#include "mem.h"
#include "string.h"
#include "sys.h"

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

	String search_path = string_build_in_stack_arena(master_arena, (String[]){
		base_path,
		SIZED_STRING("\\*.*"),
		{ 0 }
	});

	WIN32_FIND_DATA fd; 
	HANDLE directory_handle = FindFirstFile(search_path.str, &fd);

	// handle built, the string can be freed (which amounts to an integer write)
	stack_arena_empty(master_arena);

	String const current_dir = SIZED_STRING(".");
	String const parent_dir  = SIZED_STRING("..");

	EntryElement* previous_element = NULL;
	EntryElement* first_element = NULL;

	if(directory_handle == INVALID_HANDLE_VALUE)
		return first_element;
	do {
		String filename = SIZED_STRING(fd.cFileName);

		if (string_compare(filename, current_dir)
			|| string_compare(filename, parent_dir))
			continue;

		String const path = string_build_in_stack_arena(master_arena, (String[]){
			base_path,
			SIZED_STRING("\\"),
			filename,
			{ 0 }
		});
		if (!path.str)  // allocation failed, string won't fit in memory
			continue;

		EntryElement* entry = STACK_ARENA_ALLOC(EntryElement, master_arena);
		if (!entry)     // entry allocation failed
			break;      // there's no point iterating further

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
	return first_element;
}

int main() {
	// configuration
	size_t const scratch_size = 4096;
	// our entire malloc allowance
	sys_init();
	void* true_dynamic_memory = sys_alloc(scratch_size);

	if (!true_dynamic_memory) {
		size_t const last_error = GetLastError();
		return last_error;
	}

	// arena setup to organize malloc use
	StackArena arena = stack_arena_generate(true_dynamic_memory, scratch_size);

	EntryElement const* const first_entry = enumerate_fs_path(SIZED_STRING("."), &arena);

	size_t const scratch_consumed_memory = arena.used;
	printf("\nused %zu bytes", scratch_consumed_memory);

	EntryElement const* entry = first_entry;
	do {
		char* entry_type_string[] = {
			"file:",
			"dir: "
		};
		if (entry->item.path)
			printf("%s %s\n", entry_type_string[entry->item.type], entry->item.path);
		else
			puts("<ERROR>");	
	} while((entry = entry->next));

	puts("done");
	stack_arena_empty(&arena);
	return 0;
}
