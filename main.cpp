#include "main.h"

#ifdef _DEBUG
VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) {
	Globals* g = static_cast<Globals*>(user_data);

	SDL_LogCategory category = SDL_LOG_CATEGORY_GPU;
	SDL_LogPriority priority = SDL_LOG_PRIORITY_INVALID;
	if (HasFlag(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_VERBOSE;
	}
	else if (HasFlag(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_ERROR;
	}
	else if (HasFlag(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_WARN;
	}
	else if (HasFlag(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_INFO;
	}
	SDL_LogMessage(category, priority, "Vulkan: %s", data->pMessage);

	return VK_FALSE;
}
#endif

// TODO: Search for supported extensions.
#ifdef _DEBUG
static const char* const g_vk_layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
static const char* const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils" };
static const char* const g_vk_device_extensions[] = { "VK_KHR_swapchain" };
#endif
#ifndef _DEBUG
static const char* const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
static const char* const g_vk_device_extensions[] = { "VK_KHR_swapchain" };
#endif

// TODO
static const F32 g_queue_priorities[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	CHECK(SDL_Init(SDL_INIT_VIDEO));

	Globals* g;
	{
		void* memory = malloc(Megabyte(16));
		Arena arena_perm = { memory, Megabyte(8) };
		Arena arena_temp = { reinterpret_cast<void*>(reinterpret_cast<UInt>(memory) + Megabyte(8)), Megabyte(8) };
		g = Alloc(&arena_perm, Globals);
		*appstate = g;
		g->arena_perm = arena_perm;
		g->arena_temp = arena_temp;
	}


	SDL_WindowFlags flags = SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY;
	// TODO
	I32 w = 1920;
	I32 h = 1080;
	g->window = SDL_CreateWindow("GPU Journey", w, h, flags); assert(g->window);

	{
		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()); assert(vkGetInstanceProcAddr);
		volkInitializeCustom(vkGetInstanceProcAddr);
	}

#if 0
	VK_CHECK(vkEnumerateInstanceLayerProperties(&g->vk_supported_layer_count, nullptr));
	g->vk_supported_layers = Alloc<VkLayerProperties>(&g->arena_perm, g->vk_supported_layer_count);
	VK_CHECK(vkEnumerateInstanceLayerProperties(&g->vk_supported_layer_count, g->vk_supported_layers));

#ifdef VK_LAYERS
	g->vk_layers = Alloc<const char*>(&g->arena_perm, g->vk_supported_layer_count);
	for (U32 supported_layer_idx = 0; supported_layer_idx < g->vk_supported_layer_count; supported_layer_idx++) {
		char* layer_name = g->vk_supported_layers[supported_layer_idx].layerName;
		SDL_Log("layers[%u].name = %s", supported_layer_idx, g->vk_supported_layers[supported_layer_idx].layerName);

		if (strcmp(layer_name, "VK_LAYER_KHRONOS_validation") == 0) {
			g->vk_layers[g->vk_layer_count] = "VK_LAYER_KHRONOS_validation";
			g->vk_layer_count++;
		} 
		else if (strcmp(layer_name, "VK_LAYER_LUNARG_monitor") == 0) {
			g->vk_layers[g->vk_layer_count] = "VK_LAYER_LUNARG_monitor";
			g->vk_layer_count++;
		}
	}
#endif

	{
		U32 vk_supported_extension_count;
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &vk_supported_extension_count, nullptr));
		VkExtensionProperties* vk_supported_extensions = Alloc<VkExtensionProperties>(&g->arena_temp, vk_supported_extension_count);
		VK_CHECK(vkEnumerateInstanceExtensionProperties(nullptr, &vk_supported_extension_count, vk_supported_extensions));

		g->vk_instance_extensions = Alloc<const char*>(&g->arena_perm, vk_supported_extension_count);

		for (U32 supported_extension_idx = 0; supported_extension_idx < vk_supported_extension_count; supported_extension_idx++) {
			const char* extension = vk_supported_extensions[supported_extension_idx].extensionName; // VK_KHR_surface VK_KHR_win32_surface
			if (strcmp(extension, "VK_KHR_surface") == 0) {
				g->vk_instance_extensions[supported_extension_idx] = "VK_KHR_surface";
				g->vk_instance_extension_count++;
			}
			else if (strcmp(extension, "VK_KHR_win32_surface") == 0) {
				g->vk_instance_extensions[supported_extension_idx] = "VK_KHR_win32_surface";
				g->vk_instance_extension_count++;
			}
			else if (strcmp(extension, "VK_EXT_debug_utils") == 0) {
				g->vk_instance_extensions[supported_extension_idx] = "VK_EXT_debug_utils";
				g->vk_instance_extension_count++;
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
		create_info.enabledLayerCount = ArraySize(g_vk_layers);
		create_info.ppEnabledLayerNames = g_vk_layers;
#endif
		create_info.enabledExtensionCount = ArraySize(g_vk_instance_extensions);
		create_info.ppEnabledExtensionNames = g_vk_instance_extensions;

#ifdef _DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debug_info = { VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT };
		debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT| VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
		debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		debug_info.pfnUserCallback = vk_debug_callback;
		debug_info.pUserData = g;

		create_info.pNext = &debug_info;
#endif

		VK_CHECK(vkCreateInstance(&create_info, nullptr, &g->vk_instance));
	}
	
	volkLoadInstanceOnly(g->vk_instance);

	{
		// TODO
		U32 physical_device_count = 1;
		VK_CHECK(vkEnumeratePhysicalDevices(g->vk_instance, &physical_device_count, &g->vk_physical_device));
		vkGetPhysicalDeviceProperties(g->vk_physical_device, &g->physical_device_properties);
	}

	CHECK(SDL_Vulkan_CreateSurface(g->window, g->vk_instance, nullptr, &g->vk_surface));
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g->vk_physical_device, g->vk_surface, &g->vk_surface_capabilities));
	vkGetPhysicalDeviceSurfaceFormatsKHR(g->vk_physical_device, g->vk_surface, &g->vk_surface_format_count, nullptr);
	g->vk_surface_formats = AllocArray(&g->arena_perm, VkSurfaceFormatKHR, g->vk_surface_format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(g->vk_physical_device, g->vk_surface, &g->vk_surface_format_count, g->vk_surface_formats);
	
#if 0
	{
		U32 vk_supported_extension_count;
		vkEnumerateDeviceExtensionProperties(g->vk_physical_device, nullptr, &vk_supported_extension_count, nullptr);
		VkExtensionProperties* vk_supported_extensions = Alloc<VkExtensionProperties>(&g->arena_temp, vk_supported_extension_count);
		vkEnumerateDeviceExtensionProperties(g->vk_physical_device, nullptr, &vk_supported_extension_count, vk_supported_extensions);

		g->vk_device_extensions = Alloc<const char*>(&g->arena_perm, vk_supported_extension_count);
		for (U32 device_extension_idx = 0; device_extension_idx < vk_supported_extension_count; device_extension_idx++) {
			const char* extension = vk_supported_extensions[device_extension_idx].extensionName;
			SDL_Log("extensions[%u] = %s", device_extension_idx, extension);
			if (strcmp(extension, "VK_KHR_swapchain") == 0) {
				g->vk_device_extensions[g->vk_device_extension_count] = "VK_KHR_swapchain";
				g->vk_device_extension_count++;
			}
		}
	}
#endif

	{
		vkGetPhysicalDeviceQueueFamilyProperties(g->vk_physical_device, &g->vk_queue_family_count, nullptr);
		VkQueueFamilyProperties* queue_family_properties = AllocArray(&g->arena_temp, VkQueueFamilyProperties, g->vk_queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(g->vk_physical_device, &g->vk_queue_family_count, queue_family_properties);

		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(g->vk_physical_device, &physical_device_features);

		{
			VkDeviceQueueCreateInfo* queue_infos = AllocArray(&g->arena_temp, VkDeviceQueueCreateInfo, g->vk_queue_family_count);
			for (U32 queue_family_idx = 0; queue_family_idx < g->vk_queue_family_count; queue_family_idx++) {
				VkDeviceQueueCreateInfo* info = &queue_infos[queue_family_idx];
				info->sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				info->pNext = nullptr;
				info->flags = 0;
				info->queueFamilyIndex = queue_family_idx;
				info->queueCount = queue_family_properties[queue_family_idx].queueCount;
				info->pQueuePriorities = g_queue_priorities;
			}

			VkDeviceCreateInfo info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
			info.queueCreateInfoCount = g->vk_queue_family_count;
			info.pQueueCreateInfos = queue_infos;
			info.ppEnabledExtensionNames = g_vk_device_extensions;
			info.enabledExtensionCount = ArraySize(g_vk_device_extensions);
			info.pEnabledFeatures = &physical_device_features;

			VK_CHECK(vkCreateDevice(g->vk_physical_device, &info, nullptr, &g->vk_device));
		}

		volkLoadDevice(g->vk_device);

		g->vk_queue_families = AllocArray(&g->arena_perm, VkQueueFamily, g->vk_queue_family_count);
		for (U32 queue_family_idx = 0; queue_family_idx < g->vk_queue_family_count; queue_family_idx++) {
			VkQueueFamily* queue_family = &g->vk_queue_families[queue_family_idx];
			queue_family->properties = queue_family_properties[queue_family_idx];
			queue_family->queues = AllocArray(&g->arena_perm, VkQueue, queue_family->properties.queueCount);
			for (U32 queue_idx = 0; queue_idx < queue_family->properties.queueCount; queue_idx++) {
				vkGetDeviceQueue(g->vk_device, queue_family_idx, queue_idx, &queue_family->queues[queue_idx]);
			}
		}
	}

	{
		g->vk_swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		g->vk_swapchain_info.surface = g->vk_surface;
		g->vk_swapchain_info.minImageCount = Min(g->vk_surface_capabilities.minImageCount + 1, g->vk_surface_capabilities.maxImageCount);
		g->vk_swapchain_info.imageFormat = g->vk_surface_formats[1].format; // TODO
		g->vk_swapchain_info.imageColorSpace = g->vk_surface_formats[1].colorSpace; // TODO
		g->vk_swapchain_info.imageExtent = g->vk_surface_capabilities.currentExtent;
		g->vk_swapchain_info.imageArrayLayers = 1;
		g->vk_swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		g->vk_swapchain_info.preTransform = g->vk_surface_capabilities.currentTransform;
		g->vk_swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		g->vk_swapchain_info.presentMode = VK_PRESENT_MODE_FIFO_KHR;
		g->vk_swapchain_info.clipped = VK_TRUE;
		VK_CHECK(vkCreateSwapchainKHR(g->vk_device, &g->vk_swapchain_info, nullptr, &g->vk_swapchain));

		VK_CHECK(vkGetSwapchainImagesKHR(g->vk_device, g->vk_swapchain, &g->vk_swapchain_image_count, nullptr));
		g->vk_swapchain_images = AllocArray(&g->arena_perm, VkImage, g->vk_swapchain_image_count);
		VK_CHECK(vkGetSwapchainImagesKHR(g->vk_device, g->vk_swapchain, &g->vk_swapchain_image_count, g->vk_swapchain_images));

		g->vk_swapchain_image_views = AllocArray(&g->arena_perm, VkImageView, g->vk_swapchain_image_count);
		for (U32 swapchain_image_idx = 0; swapchain_image_idx < g->vk_swapchain_image_count; swapchain_image_idx++) {
			VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			info.image = g->vk_swapchain_images[swapchain_image_idx];
			info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			info.format = g->vk_swapchain_info.imageFormat;
			info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			info.subresourceRange.levelCount = 1;
			info.subresourceRange.layerCount = 1;
			VK_CHECK(vkCreateImageView(g->vk_device, &info, nullptr, &g->vk_swapchain_image_views[swapchain_image_idx]));
		}
	}

	{
		VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
		VK_CHECK(vkCreateCommandPool(g->vk_device, &info, nullptr, &g->vk_command_pool));
	}

	{
		VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		info.commandPool = g->vk_command_pool;
		info.commandBufferCount = 1;
		VK_CHECK(vkAllocateCommandBuffers(g->vk_device, &info, &g->vk_command_buffer));
	}

#if 0
	{
		VkAttachmentDescription a0 = {};
		a0.format = g->vk_swapchain_info.imageFormat;

		VkSubpassDescription subpass_desc = {};


		VkRenderPassCreateInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		info.attachmentCount = 1;
		info.pAttachments = &a0;

	}

	{
		VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };

	}
#endif

	g->vk_queue = g->vk_queue_families[0].queues[0]; // TODO

	Reset(&g->arena_temp);

	return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	Globals* g = static_cast<Globals*>(appstate);

#if 0
	// begin frame
	{
		VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(g->vk_command_buffer, &info));
	}

	// end frame
	{
		VK_CHECK(vkEndCommandBuffer(g->vk_command_buffer));
		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &g->vk_command_buffer;
		VK_CHECK(vkQueueSubmit(g->vk_queue, 1, &submit_info, VK_NULL_HANDLE));
	}
#endif

	return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
	Globals* g = static_cast<Globals*>(appstate);

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

void* _Alloc(Arena* arena, Int size, Int align) {
	assert(align % 2 == 0);
	Int data = reinterpret_cast<Int>(arena->data) + arena->len;
	Int aligned_data = (data + align - 1) & -align;
	Int final_data = aligned_data + size;
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