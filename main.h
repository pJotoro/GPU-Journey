#pragma once

#include <Volk/volk.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))
#define assert(expr) SDL_assert(expr)
#define CLAMP(X,A,B) SDL_clamp(X,A,B)
#define MAX(X,Y) SDL_max(X,Y)
#define MIN(X,Y) SDL_min(X,Y)

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define forceinline __forceinline

#define STMT(X) do {X} while (false)
#define CHECK(EXPR) STMT(bool ok = EXPR; assert(ok);)
#define VK_CHECK(EXPR) STMT(VkResult res = EXPR; assert(res == VK_SUCCESS);)

struct Arena {
	void* data;
	int64_t cap;
	int64_t len;
};

void* Alloc(Arena* arena, int64_t size, int64_t align);
void Reset(Arena* arena);
void Free(Arena* arena);

template <typename T>
forceinline T* Alloc(Arena* arena) {
	return static_cast<T*>(Alloc(arena, sizeof(T), alignof(T)));
}
template <typename T>
forceinline T* Alloc(Arena* arena, int64_t count) {
	return static_cast<T*>(Alloc(arena, sizeof(T)*count, alignof(T)));
}

// Why is this not already a struct in Vulkan? It would make things a lot easier.
struct VkQueueFamily {
	VkQueueFamilyProperties properties;
	VkQueue* queues;
};

struct Context {
	SDL_Window* window;

	Arena arena_perm;
	Arena arena_temp;

	// ----- Vulkan -----
	VkInstance vk_instance;

#if 0
	uint32_t vk_supported_layer_count;
	VkLayerProperties* vk_supported_layers;

	uint32_t vk_layer_count;
	const char* const* vk_layers;

	uint32_t vk_instance_extension_count;
	const char* const* vk_instance_extensions;
#endif

	VkPhysicalDevice vk_physical_device;
	VkPhysicalDeviceProperties physical_device_properties;
	
	VkSurfaceKHR vk_surface;
	VkSurfaceCapabilitiesKHR vk_surface_capabilities;
	uint32_t vk_surface_format_count;
	VkSurfaceFormatKHR* vk_surface_formats;

	uint32_t vk_queue_family_count;
	VkQueueFamily* vk_queue_families;
	VkQueue vk_queue;

#if 0
	uint32_t vk_device_extension_count;
	const char** vk_device_extensions;
#endif

	VkDevice vk_device;

	VkSwapchainKHR vk_swapchain; // TODO

	VkCommandPool vk_command_pool;
	VkCommandBuffer vk_command_buffer;

	VkRenderPass vk_render_pass;
	// ------------------
};