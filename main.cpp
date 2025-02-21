#include "main.h"

VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) {
	//Context* ctx = static_cast<Context*>(user_data);
	//SDL_Log("Vulkan: %s", data->pMessage);
	return VK_FALSE;
}

// TODO: Search for supported extensions.
#ifdef _DEBUG
const char* const g_vk_layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
#endif
const char* const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils" };
const char* const g_vk_device_extensions[] = { "VK_KHR_swapchain" };

// TODO
const float queue_priorities[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	CHECK(SDL_Init(SDL_INIT_VIDEO));

	Context* ctx = static_cast<Context*>(calloc(1, sizeof(Context)));
	*appstate = ctx;
	ctx->arena_perm = Arena{ malloc(MEGABYTE(8)), MEGABYTE(8), 0 };
	ctx->arena_temp = Arena{ malloc(MEGABYTE(8)), MEGABYTE(8), 0 };

	SDL_WindowFlags flags = SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY;
	// TODO
	int w = 1920;
	int h = 1080;
	ctx->window = SDL_CreateWindow("GPU Journey", w, h, flags); assert(ctx->window);

	{
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()); assert(vkGetInstanceProcAddr);
		volkInitializeCustom(vkGetInstanceProcAddr);
	}

#if 0
	VK_CHECK(vkEnumerateInstanceLayerProperties(&ctx->vk_supported_layer_count, nullptr));
	ctx->vk_supported_layers = Alloc<VkLayerProperties>(&ctx->arena_perm, ctx->vk_supported_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&ctx->vk_supported_layer_count, ctx->vk_supported_layers));

#ifdef VK_LAYERS
	ctx->vk_layers = Alloc<const char*>(&ctx->arena_perm, ctx->vk_supported_layer_count);
	for (uint32_t supported_layer_idx = 0; supported_layer_idx < ctx->vk_supported_layer_count; supported_layer_idx++) {
		char* layer_name = ctx->vk_supported_layers[supported_layer_idx].layerName;
		SDL_Log("layers[%u].name = %s", supported_layer_idx, ctx->vk_supported_layers[supported_layer_idx].layerName);

		if (strcmp(layer_name, "VK_LAYER_KHRONOS_validation") == 0) {
			ctx->vk_layers[ctx->vk_layer_count] = "VK_LAYER_KHRONOS_validation";
			ctx->vk_layer_count++;
		} 
		else if (strcmp(layer_name, "VK_LAYER_LUNARG_monitor") == 0) {
			ctx->vk_layers[ctx->vk_layer_count] = "VK_LAYER_LUNARG_monitor";
			ctx->vk_layer_count++;
		}
	}
#endif

	{
		uint32_t vk_supported_extension_count;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &vk_supported_extension_count, nullptr));
		VkExtensionProperties* vk_supported_extensions = Alloc<VkExtensionProperties>(&ctx->arena_temp, vk_supported_extension_count);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &vk_supported_extension_count, vk_supported_extensions));

		ctx->vk_instance_extensions = Alloc<const char*>(&ctx->arena_perm, vk_supported_extension_count);

		for (uint32_t supported_extension_idx = 0; supported_extension_idx < vk_supported_extension_count; supported_extension_idx++) {
			const char* extension = vk_supported_extensions[supported_extension_idx].extensionName; // VK_KHR_surface VK_KHR_win32_surface
			if (strcmp(extension, "VK_KHR_surface") == 0) {
				ctx->vk_instance_extensions[supported_extension_idx] = "VK_KHR_surface";
				ctx->vk_instance_extension_count++;
			}
			else if (strcmp(extension, "VK_KHR_win32_surface") == 0) {
				ctx->vk_instance_extensions[supported_extension_idx] = "VK_KHR_win32_surface";
				ctx->vk_instance_extension_count++;
			}
			else if (strcmp(extension, "VK_EXT_debug_utils") == 0) {
				ctx->vk_instance_extensions[supported_extension_idx] = "VK_EXT_debug_utils";
				ctx->vk_instance_extension_count++;
			}
		}
	}
#endif
	
	{
		VkApplicationInfo app_info = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
		VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		
		app_info.pApplicationName = "GPU Journey";
		app_info.applicationVersion = VK_API_VERSION_1_0;
		app_info.pEngineName = "GPU Journey";
		app_info.engineVersion = VK_API_VERSION_1_0;
		app_info.apiVersion = VK_API_VERSION_1_0;
		
		create_info.pApplicationInfo = &app_info;
#ifdef _DEBUG
		create_info.enabledLayerCount = ARRAY_SIZE(g_vk_layers);
		create_info.ppEnabledLayerNames = g_vk_layers;
#endif
		create_info.enabledExtensionCount = ARRAY_SIZE(g_vk_instance_extensions);
		create_info.ppEnabledExtensionNames = g_vk_instance_extensions;

		VkDebugUtilsMessengerCreateInfoEXT debug_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT;
		debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_info.pfnUserCallback = vk_debug_callback;
		debug_info.pUserData = ctx;

		create_info.pNext = &debug_info;

		VK_CHECK(vkCreateInstance(&create_info, nullptr, &ctx->vk_instance));
	}
	
	volkLoadInstanceOnly(ctx->vk_instance);

	{
		// TODO
		uint32_t physical_device_count = 1;
		VK_CHECK(vkEnumeratePhysicalDevices(ctx->vk_instance, &physical_device_count, &ctx->vk_physical_device));
		vkGetPhysicalDeviceProperties(ctx->vk_physical_device, &ctx->physical_device_properties);
	}

	CHECK(SDL_Vulkan_CreateSurface(ctx->window, ctx->vk_instance, nullptr, &ctx->vk_surface));
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_physical_device, ctx->vk_surface, &ctx->vk_surface_capabilities));
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, ctx->vk_surface, &ctx->vk_surface_format_count, nullptr);
	ctx->vk_surface_formats = Alloc<VkSurfaceFormatKHR>(&ctx->arena_perm, ctx->vk_surface_format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, ctx->vk_surface, &ctx->vk_surface_format_count, ctx->vk_surface_formats);

#if 0
	{
		uint32_t vk_supported_extension_count;
		vkEnumerateDeviceExtensionProperties(ctx->vk_physical_device, nullptr, &vk_supported_extension_count, nullptr);
		VkExtensionProperties* vk_supported_extensions = Alloc<VkExtensionProperties>(&ctx->arena_temp, vk_supported_extension_count);
		vkEnumerateDeviceExtensionProperties(ctx->vk_physical_device, nullptr, &vk_supported_extension_count, vk_supported_extensions);

		ctx->vk_device_extensions = Alloc<const char*>(&ctx->arena_perm, vk_supported_extension_count);
		for (uint32_t device_extension_idx = 0; device_extension_idx < vk_supported_extension_count; device_extension_idx++) {
			const char* extension = vk_supported_extensions[device_extension_idx].extensionName;
			SDL_Log("extensions[%u] = %s", device_extension_idx, extension);
			if (strcmp(extension, "VK_KHR_swapchain") == 0) {
				ctx->vk_device_extensions[ctx->vk_device_extension_count] = "VK_KHR_swapchain";
				ctx->vk_device_extension_count++;
			}
		}
	}
#endif

	{
		vkGetPhysicalDeviceQueueFamilyProperties(ctx->vk_physical_device, &ctx->vk_queue_family_count, nullptr);
		VkQueueFamilyProperties* queue_family_properties = Alloc<VkQueueFamilyProperties>(&ctx->arena_temp, ctx->vk_queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(ctx->vk_physical_device, &ctx->vk_queue_family_count, queue_family_properties);

		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(ctx->vk_physical_device, &physical_device_features);

		{
			VkDeviceQueueCreateInfo* queue_infos = Alloc<VkDeviceQueueCreateInfo>(&ctx->arena_temp, ctx->vk_queue_family_count);
			for (uint32_t queue_family_idx = 0; queue_family_idx < ctx->vk_queue_family_count; queue_family_idx++) {
				VkDeviceQueueCreateInfo* info = &queue_infos[queue_family_idx];
				info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info->pNext = nullptr;
				info->flags = 0;
				info->queueFamilyIndex = queue_family_idx;
				info->queueCount = queue_family_properties[queue_family_idx].queueCount;
				info->pQueuePriorities = queue_priorities;
			}

			VkDeviceCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			info.queueCreateInfoCount = ctx->vk_queue_family_count;
			info.pQueueCreateInfos = queue_infos;
			info.ppEnabledExtensionNames = g_vk_device_extensions;
			info.enabledExtensionCount = ARRAY_SIZE(g_vk_device_extensions);
			info.pEnabledFeatures = &physical_device_features;

			VK_CHECK(vkCreateDevice(ctx->vk_physical_device, &info, nullptr, &ctx->vk_device));
		}

		volkLoadDevice(ctx->vk_device);

		ctx->vk_queue_families = Alloc<VkQueueFamily>(&ctx->arena_perm, ctx->vk_queue_family_count);
		for (uint32_t queue_family_idx = 0; queue_family_idx < ctx->vk_queue_family_count; queue_family_idx++) {
			VkQueueFamily* queue_family = &ctx->vk_queue_families[queue_family_idx];
			queue_family->properties = queue_family_properties[queue_family_idx];
			queue_family->queues = Alloc<VkQueue>(&ctx->arena_perm, queue_family->properties.queueCount);
			for (uint32_t queue_idx = 0; queue_idx < queue_family->properties.queueCount; queue_idx++) {
				vkGetDeviceQueue(ctx->vk_device, queue_family_idx, queue_idx, &queue_family->queues[queue_idx]);
			}
		}
	}

	{
		VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		info.surface = ctx->vk_surface;
		info.minImageCount = MIN(ctx->vk_surface_capabilities.minImageCount + 1, ctx->vk_surface_capabilities.maxImageCount);
		info.imageFormat = ctx->vk_surface_formats[1].format; // TODO
		info.imageColorSpace = ctx->vk_surface_formats[1].colorSpace; // TODO
		info.imageExtent = ctx->vk_surface_capabilities.currentExtent;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.preTransform = ctx->vk_surface_capabilities.currentTransform;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode = VK_PRESENT_MODE_FIFO_KHR; // TODO
		info.clipped = VK_TRUE;
		VK_CHECK(vkCreateSwapchainKHR(ctx->vk_device, &info, nullptr, &ctx->vk_swapchain));
	}

	{
		VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		VK_CHECK(vkCreateCommandPool(ctx->vk_device, &info, nullptr, &ctx->vk_command_pool));
	}

	{
		VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		info.commandPool = ctx->vk_command_pool;
		info.commandBufferCount = 1;
		VK_CHECK(vkAllocateCommandBuffers(ctx->vk_device, &info, &ctx->vk_command_buffer));
	}

	{
		VkRenderPassCreateInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };

	}

	ctx->vk_queue = ctx->vk_queue_families[0].queues[0]; // TODO

	Reset(&ctx->arena_temp);

	return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	Context* ctx = static_cast<Context*>(appstate);

#if 0
	// begin frame
	{
		VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(ctx->vk_command_buffer, &info));
	}

	// end frame
	{
		VK_CHECK(vkEndCommandBuffer(ctx->vk_command_buffer));
		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &ctx->vk_command_buffer;
		VK_CHECK(vkQueueSubmit(ctx->vk_queue, 1, &submit_info, VK_NULL_HANDLE));
	}
#endif
	

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	Context* ctx = static_cast<Context*>(appstate);

	switch (event->type) {
	case SDL_EVENT_KEY_DOWN:
		if (event->key.key == SDLK_ESCAPE) {
			return SDL_APP_SUCCESS;
		}
		break;
	case SDL_EVENT_QUIT:
		return SDL_APP_SUCCESS;
	}
	return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {

}

void* Alloc(Arena* arena, int64_t size, int64_t align) {
	assert(align % 2 == 0);
	int64_t data = reinterpret_cast<int64_t>(arena->data) + arena->len;
	int64_t aligned_data = (data + align - 1) & -align;
	int64_t final_data = aligned_data + size;
	arena->len += final_data - data;
	assert(arena->len < arena->cap);
	return reinterpret_cast<void*>(aligned_data);
}

void Reset(Arena* arena) {
	arena->len = 0;
}

void Free(Arena* arena) {
	free(arena->data);
	*arena = {};
}