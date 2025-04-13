#pragma once

#include <Volk/volk.h>

#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_vulkan.h>
#include <SDL3/SDL_image.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "tiny_obj_loader.h"

#include <array>
#include <span>

#include "arena.h"

#define KILOBYTE(X) ((X)*1024LL)
#define MEGABYTE(X) (KILOBYTE(X)*1024LL)
#define GIGABYTE(X) (MEGABYTE(X)*1024LL)
#define TERABYTE(X) (GIGABYTE(X)*1024LL)

#define FORCEINLINE __forceinline

#define STMT(X) do {X} while (false)

#define HAS_FLAG(FLAGS, FLAG) ((FLAGS) & (FLAG))

const uint32_t MAX_FRAMES_IN_FLIGHT = 2;

struct Vertex {
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec2 tex_coord;
};

struct UniformBufferObject {
	alignas(16) glm::mat4 model, view, proj;
};

enum class WindowVisible {
	HIDE,
	SHOW,
	SHOWN,
};

struct Context {
	SDL_Window* window;
	WindowVisible window_visible;

	Arena arena_perm;
	Arena arena_temp;

	// ----- Vulkan -----
	VkInstance vk_instance;

	VkPhysicalDevice vk_physical_device;
	VkPhysicalDeviceProperties vk_physical_device_properties;
	VkPhysicalDeviceMemoryProperties vk_physical_device_memory_properties;

	VkSurfaceKHR vk_surface;
	VkSurfaceCapabilitiesKHR vk_surface_capabilities;
	std::span<VkSurfaceFormatKHR> vk_surface_formats;

	std::span<VkQueueFamilyProperties> vk_queue_family_properties;
	std::span<std::span<VkQueue>> vk_queues;
	VkQueue vk_graphics_queue;

	VkDevice vk_device;

	// TODO: Swapchain recreation.
	VkSwapchainKHR vk_swapchain;
	VkSwapchainCreateInfoKHR vk_swapchain_info;

	std::span<VkImage> vk_swapchain_images;
	std::span<VkImageView> vk_swapchain_image_views;
	std::span<VkFramebuffer> vk_framebuffers;

	VkShaderModule vk_vs_module;
	VkShaderModule vk_fs_module;

	VkViewport vk_viewport;
	VkRect2D vk_scissor;

	VkRenderPass vk_render_pass;
	VkDescriptorSetLayout vk_descriptor_set_layout;
	VkPipelineLayout vk_pipeline_layout;
	VkPipeline vk_graphics_pipeline;

	VkCommandPool vk_command_pool;

	VkDescriptorPool vk_descriptor_pool;
	std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> vk_descriptor_set;

	std::array<VkCommandBuffer, MAX_FRAMES_IN_FLIGHT> vk_command_buffer;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> vk_sem_image_available;
	std::array<VkSemaphore, MAX_FRAMES_IN_FLIGHT> vk_sem_render_finished;
	std::array<VkFence, MAX_FRAMES_IN_FLIGHT> vk_fence_in_flight;

	size_t vk_current_frame;

	VkBuffer vk_vertex_buffer;
	VkDeviceMemory vk_vertex_buffer_memory;

	VkBuffer vk_index_buffer;
	VkDeviceMemory vk_index_buffer_memory;

	VkBuffer vk_staging_buffer;
	VkDeviceMemory vk_staging_buffer_memory;
	bool vk_staged;

	VkBuffer vk_uniform_buffer;
	VkDeviceMemory vk_uniform_buffer_memory;
	void* vk_uniform_buffer_mapped;

	VkImage vk_depth_stencil_image;
	VkDeviceMemory vk_depth_stencil_image_memory;
	VkImageView vk_depth_stencil_image_view;

	int64_t start_time;

	SDL_Surface* surf;
	VkImageCreateInfo vk_image_info;
	VkImage vk_image;
	VkDeviceMemory vk_image_memory;
	VkImageView vk_image_view;

	VkSampler vk_sampler;

	uint32_t find_memory_type(uint32_t memory_types, VkMemoryPropertyFlags properties);

	// ------------------
};