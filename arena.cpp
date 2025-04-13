#include "arena.h"

#include <SDL3/SDL_assert.h>

#pragma warning(push, 4)

void* Arena::alloc(size_t size, size_t align) {
	SDL_assert(align % 2 == 0);
	uintptr_t _data = reinterpret_cast<uintptr_t>(data) + len;
	uintptr_t aligned_data = (_data + align) & ~align;
	
	uintptr_t final_data = aligned_data + size;
	len += final_data - _data;
	SDL_assert(len < cap);
	return reinterpret_cast<void*>(aligned_data);
}

std::u8string_view Arena::alloc_string(size_t string_len) {
	char8_t* ptr = alloc<char8_t>(string_len);
	return { ptr, string_len };
}

void Arena::reset() {
	this->len = 0;
}

#pragma warning(pop)