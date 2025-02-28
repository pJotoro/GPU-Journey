#pragma once

#include <Volk/volk.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <HandmadeMath.h>

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t S8;
typedef int16_t S16;
typedef int32_t I32;
typedef int64_t I64;
typedef uint64_t UInt;
typedef int64_t Int;
typedef float F32;
typedef double F64;

#define ArraySize(A) (sizeof(A)/sizeof(A[0]))
#define assert(expr) SDL_assert(expr)
#define Clamp(X,A,B) SDL_clamp(X,A,B)
#define Max(X,Y) SDL_max(X,Y)
#define Min(X,Y) SDL_min(X,Y)

#define Kilobyte(X) ((X)*1024LL)
#define Megabyte(X) (Kilobyte(X)*1024LL)
#define Gigabyte(X) (Megabyte(X)*1024LL)
#define Terabyte(X) (Gigabyte(X)*1024LL)

#define forceinline __forceinline

#define STMT(X) do {X} while (false)
#define CHECK(EXPR) STMT(bool ok = EXPR; assert(ok);)
#define VK_CHECK(EXPR) STMT(VkResult res = EXPR; assert(res == VK_SUCCESS);)

#define HasFlag(FLAGS,FLAG) ((FLAGS & FLAG) != 0)

struct Arena {
	void* data;
	Int cap;
	Int len;
};

void* _Alloc(Arena* arena, Int size, Int align);
void Reset(Arena* arena);
void Free(Arena* arena);

#define Alloc(A,T) static_cast<T*>(_Alloc(A, sizeof(T), alignof(T)))
#define AllocArray(A,T,C) static_cast<T*>(_Alloc(A, sizeof(T) * C, alignof(T)))

// Why is this not already a struct in Vulkan? It would make things a lot easier.
struct VkQueueFamily {
	VkQueueFamilyProperties properties;
	VkQueue* queues;
};

struct Globals {
	SDL_Window* window;

	Arena arena_perm;
	Arena arena_temp;

	// ----- Vulkan -----
	VkInstance vk_instance;

#if 0
	VkLayerProperties* vk_supported_layers;
	U32 vk_supported_layer_count;

	const char* const* vk_layers;
	U32 vk_layer_count;

	const char* const* vk_instance_extensions;
	U32 vk_instance_extension_count;
#endif

	VkPhysicalDevice vk_physical_device;
	VkPhysicalDeviceProperties physical_device_properties;
	
	VkSurfaceKHR vk_surface;
	VkSurfaceCapabilitiesKHR vk_surface_capabilities;
	VkSurfaceFormatKHR* vk_surface_formats;
	U32 vk_surface_format_count;

	VkQueueFamily* vk_queue_families;
	U32 vk_queue_family_count;
	VkQueue vk_queue;

#if 0
	const char** vk_device_extensions;
	U32 vk_device_extension_count;
#endif

	VkDevice vk_device;

	VkSwapchainKHR vk_swapchain;
	VkSwapchainCreateInfoKHR vk_swapchain_info;
	VkImage* vk_swapchain_images;
	VkImageView* vk_swapchain_image_views;
	U32 vk_swapchain_image_count;

	VkRenderPass vk_render_pass; // TODO
	VkFramebuffer vk_framebuffer; // TODO

	VkCommandPool vk_command_pool;
	VkCommandBuffer vk_command_buffer;
	// ------------------
};