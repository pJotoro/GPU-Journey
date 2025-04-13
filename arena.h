#pragma once

#include <span>
#include <string_view>

struct Arena {
	void* alloc(size_t size, size_t align);

	template <typename T>
	T* alloc() {
		return reinterpret_cast<T*>(alloc(sizeof(T), alignof(T)));
	}

	template <typename T>
	T* alloc(size_t count) {
		return reinterpret_cast<T*>(alloc(sizeof(T) * count, alignof(T)));
	}

	template <typename T>
	std::span<T> alloc_span(size_t count) {
		T* ptr = reinterpret_cast<T*>(alloc(sizeof(T) * count, alignof(T)));
		return { ptr, count };
	}

	std::u8string_view alloc_string(size_t string_len);

	void reset();

	void* data;
	uintptr_t cap;
	uintptr_t len;
};