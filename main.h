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

#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))
#define assert(expr) SDL_assert(expr)

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define INLINE __forceinline

#define STMT(X) do {X} while (false)
#define CHECK(EXPR) STMT(bool ok = EXPR; assert(ok);)
#define VK_CHECK(EXPR) STMT(VkResult res = EXPR; assert(res == VK_SUCCESS);)

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG))

struct Arena {
	void* data;
	Int cap;
	Int len;
};

void* _alloc(Arena* arena, Int size, Int align);
void reset(Arena* arena);
void free(Arena* arena);

#define ALLOC(A, T) static_cast<T*>(_alloc(A, sizeof(T), alignof(T)))
#define ALLOC_ARRAY(A, T, C) static_cast<T*>(_alloc(A, sizeof(T) * C, alignof(T)))

// Why is this not already a struct in Vulkan? It would make things a lot easier.
struct VkQueueFamily {
	VkQueueFamilyProperties properties;
	VkQueue* queues;
};

#define MAX_FRAMES_IN_FLIGHT 2

struct Vertex {
	HMM_Vec2 pos;
	HMM_Vec3 color;
};

enum Window_Visible {
	WINDOW_VISIBLE_HIDE,
	WINDOW_VISIBLE_SHOW,
	WINDOW_VISIBLE_ALREADY_SHOWN,
};

struct Globals {
	SDL_Window* window;
	Window_Visible window_visible;

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
	VkPhysicalDeviceProperties vk_physical_device_properties;
	VkPhysicalDeviceMemoryProperties vk_physical_device_memory_properties;
	
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

	// TODO: Swapchain recreation.
	VkSwapchainKHR vk_swapchain;
	VkSwapchainCreateInfoKHR vk_swapchain_info;

	VkImage* vk_swapchain_images;
	VkImageView* vk_swapchain_image_views;
	VkFramebuffer* vk_framebuffers;
	union { // In case I forget that all of these have the same count.
		U32 vk_swapchain_image_count;
		U32 vk_swapchain_image_view_count;
		U32 vk_framebuffers_count;
	};

	VkShaderModule vk_vs_module;
	VkShaderModule vk_fs_module;

	VkViewport vk_viewport;
	VkRect2D vk_scissor;

	VkRenderPass vk_render_pass;
	VkPipelineLayout vk_pipeline_layout;
	VkPipeline vk_graphics_pipeline;

	VkCommandPool vk_command_pool;

	VkCommandBuffer vk_command_buffer[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore vk_sem_image_available[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore vk_sem_render_finished[MAX_FRAMES_IN_FLIGHT];
	VkFence vk_fence_in_flight[MAX_FRAMES_IN_FLIGHT];

	Int vk_current_frame;

	VkBuffer vk_vertex_buffer;
	VkDeviceMemory vk_vertex_buffer_memory;

	VkBuffer vk_index_buffer;
	VkDeviceMemory vk_index_buffer_memory;

	VkBuffer vk_staging_buffer;
	VkDeviceMemory vk_staging_buffer_memory;
	bool vk_staged_buffers;

	// ------------------
};

U32 find_memory_type(Globals* g, U32 memory_types, VkMemoryPropertyFlags properties);