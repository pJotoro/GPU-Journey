#include "main.h"

// TODO
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#ifdef _DEBUG
VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) {
	Globals* g = static_cast<Globals*>(user_data);

	SDL_LogCategory category = SDL_LOG_CATEGORY_GPU;
	SDL_LogPriority priority = SDL_LOG_PRIORITY_INVALID;
	if (HAS_FLAG(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_VERBOSE;
	}
	else if (HAS_FLAG(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_ERROR;
	}
	else if (HAS_FLAG(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_WARN;
	}
	else if (HAS_FLAG(severity, VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)) {
		priority = SDL_LOG_PRIORITY_INFO;
	}
	SDL_LogMessage(category, priority, data->pMessage);

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

// TODO
static const Vertex g_vertices[] = {
	{{0.0f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
	{{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}},
};

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	CHECK(SDL_Init(SDL_INIT_VIDEO));

	Globals* g;
	{
		void* memory = malloc(MEGABYTE(16));
		Arena arena_perm = { memory, MEGABYTE(8) };
		Arena arena_temp = { reinterpret_cast<void*>(reinterpret_cast<UInt>(memory) + MEGABYTE(8)), MEGABYTE(8) };
		g = ALLOC(&arena_perm, Globals);
		*appstate = g;
		*g = {};
		g->arena_perm = arena_perm;
		g->arena_temp = arena_temp;
	}


	SDL_WindowFlags flags = SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_HIDDEN;
	g->window = SDL_CreateWindow("GPU Journey", SCREEN_WIDTH, SCREEN_HEIGHT, flags); assert(g->window);

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
		create_info.enabledLayerCount = ARRAY_SIZE(g_vk_layers);
		create_info.ppEnabledLayerNames = g_vk_layers;
#endif
		create_info.enabledExtensionCount = ARRAY_SIZE(g_vk_instance_extensions);
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
		vkGetPhysicalDeviceProperties(g->vk_physical_device, &g->vk_physical_device_properties);
		vkGetPhysicalDeviceMemoryProperties(g->vk_physical_device, &g->vk_physical_device_memory_properties);
	}

	CHECK(SDL_Vulkan_CreateSurface(g->window, g->vk_instance, nullptr, &g->vk_surface));
	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(g->vk_physical_device, g->vk_surface, &g->vk_surface_capabilities));
	vkGetPhysicalDeviceSurfaceFormatsKHR(g->vk_physical_device, g->vk_surface, &g->vk_surface_format_count, nullptr);
	g->vk_surface_formats = ALLOC_ARRAY(&g->arena_perm, VkSurfaceFormatKHR, g->vk_surface_format_count);
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
		VkQueueFamilyProperties* queue_family_properties = ALLOC_ARRAY(&g->arena_temp, VkQueueFamilyProperties, g->vk_queue_family_count);
		vkGetPhysicalDeviceQueueFamilyProperties(g->vk_physical_device, &g->vk_queue_family_count, queue_family_properties);

		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(g->vk_physical_device, &physical_device_features);

		{
			VkDeviceQueueCreateInfo* queue_infos = ALLOC_ARRAY(&g->arena_temp, VkDeviceQueueCreateInfo, g->vk_queue_family_count);
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
			info.enabledExtensionCount = ARRAY_SIZE(g_vk_device_extensions);
			info.pEnabledFeatures = &physical_device_features;

			VK_CHECK(vkCreateDevice(g->vk_physical_device, &info, nullptr, &g->vk_device));
		}

		volkLoadDevice(g->vk_device);

		g->vk_queue_families = ALLOC_ARRAY(&g->arena_perm, VkQueueFamily, g->vk_queue_family_count);
		for (U32 queue_family_idx = 0; queue_family_idx < g->vk_queue_family_count; queue_family_idx++) {
			VkQueueFamily* queue_family = &g->vk_queue_families[queue_family_idx];
			queue_family->properties = queue_family_properties[queue_family_idx];
			queue_family->queues = ALLOC_ARRAY(&g->arena_perm, VkQueue, queue_family->properties.queueCount);
			for (U32 queue_idx = 0; queue_idx < queue_family->properties.queueCount; queue_idx++) {
				vkGetDeviceQueue(g->vk_device, queue_family_idx, queue_idx, &queue_family->queues[queue_idx]);
			}
		}
	}

	{
		g->vk_swapchain_info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		g->vk_swapchain_info.surface = g->vk_surface;
		g->vk_swapchain_info.minImageCount = HMM_MIN(g->vk_surface_capabilities.minImageCount + 1, g->vk_surface_capabilities.maxImageCount);
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
		g->vk_swapchain_images = ALLOC_ARRAY(&g->arena_perm, VkImage, g->vk_swapchain_image_count);
		VK_CHECK(vkGetSwapchainImagesKHR(g->vk_device, g->vk_swapchain, &g->vk_swapchain_image_count, g->vk_swapchain_images));

		g->vk_swapchain_image_views = ALLOC_ARRAY(&g->arena_perm, VkImageView, g->vk_swapchain_image_count);
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

	// CreateShaderModule
	{
		size_t len;
		void* data = SDL_LoadFile("vert.spv", &len); assert(data);

		VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		info.codeSize = len;
		info.pCode = reinterpret_cast<U32*>(data);
		VK_CHECK(vkCreateShaderModule(g->vk_device, &info, nullptr, &g->vk_vs_module));
	}

	// CreateShaderModule
	{
		size_t len;
		void* data = SDL_LoadFile("frag.spv", &len); assert(data);

		VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		info.codeSize = len;
		info.pCode = reinterpret_cast<U32*>(data);
		VK_CHECK(vkCreateShaderModule(g->vk_device, &info, nullptr, &g->vk_fs_module));
	}

	{
		VkPipelineShaderStageCreateInfo vert_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vert_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vert_info.module = g->vk_vs_module;
		vert_info.pName = "main";
		
		VkPipelineShaderStageCreateInfo frag_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		frag_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		frag_info.module = g->vk_fs_module;
		frag_info.pName = "main";

		VkPipelineShaderStageCreateInfo shader_stages[] = {
			vert_info,
			frag_info,
		};

		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
		dynamic_state_info.dynamicStateCount = ARRAY_SIZE(dynamic_states);
		dynamic_state_info.pDynamicStates = dynamic_states;

		VkVertexInputBindingDescription vertex_input_binding = {};
		vertex_input_binding.binding = 0;
		vertex_input_binding.stride = sizeof(Vertex);
		vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertex_attributes[2] = {};

		vertex_attributes[0].binding = 0;
		vertex_attributes[0].location = 0;
		vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attributes[0].offset = offsetof(Vertex, pos);

		vertex_attributes[1].binding = 0;
		vertex_attributes[1].location = 1;
		vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attributes[1].offset = offsetof(Vertex, color);

		VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding;
		vertex_input_info.vertexAttributeDescriptionCount = ARRAY_SIZE(vertex_attributes);
		vertex_input_info.pVertexAttributeDescriptions = vertex_attributes;

		// TODO: what does primitive restart do?
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
		input_assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

		g->vk_viewport.width = static_cast<F32>(g->vk_swapchain_info.imageExtent.width);
		g->vk_viewport.height = static_cast<F32>(g->vk_swapchain_info.imageExtent.height);
		g->vk_viewport.maxDepth = 1.0f;
		
		g->vk_scissor.extent = g->vk_swapchain_info.imageExtent;
		
		VkPipelineViewportStateCreateInfo viewport_info = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		viewport_info.viewportCount = 1;
		viewport_info.scissorCount = 1;

		VkPipelineRasterizationStateCreateInfo rasterization_info = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
		rasterization_info.lineWidth = 1.0f;
		rasterization_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_info.frontFace = VK_FRONT_FACE_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisample_info = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
		multisample_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineColorBlendAttachmentState blend_attachment_info = {};
		blend_attachment_info.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

		VkPipelineColorBlendStateCreateInfo blend_info = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
		blend_info.attachmentCount = 1;
		blend_info.pAttachments = &blend_attachment_info;

		VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		VK_CHECK(vkCreatePipelineLayout(g->vk_device, &pipeline_layout_info, nullptr, &g->vk_pipeline_layout));

		VkAttachmentDescription color_attachment = {};
		color_attachment.format = g->vk_swapchain_info.imageFormat;
		color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference color_attachment_ref = {};
		color_attachment_ref.attachment = 0; // index in attachments array
		color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass = {};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &color_attachment_ref;

		VkSubpassDependency subpass_dependency = {};
		subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass before and after
		subpass_dependency.dstSubpass = 0; // subpass index
		subpass_dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.srcAccessMask = 0;
		subpass_dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo render_pass_info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		render_pass_info.attachmentCount = 1;
		render_pass_info.pAttachments = &color_attachment;
		render_pass_info.subpassCount = 1;
		render_pass_info.pSubpasses = &subpass;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &subpass_dependency;
		VK_CHECK(vkCreateRenderPass(g->vk_device, &render_pass_info, nullptr, &g->vk_render_pass));

		VkGraphicsPipelineCreateInfo graphics_pipeline_info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		graphics_pipeline_info.stageCount = 2;
		graphics_pipeline_info.pStages = shader_stages;
		graphics_pipeline_info.pVertexInputState = &vertex_input_info;
		graphics_pipeline_info.pInputAssemblyState = &input_assembly_info;
		graphics_pipeline_info.pViewportState = &viewport_info;
		graphics_pipeline_info.pRasterizationState = &rasterization_info;
		graphics_pipeline_info.pMultisampleState = &multisample_info;
		graphics_pipeline_info.pColorBlendState = &blend_info;
		graphics_pipeline_info.pDynamicState = &dynamic_state_info;
		graphics_pipeline_info.layout = g->vk_pipeline_layout;
		graphics_pipeline_info.renderPass = g->vk_render_pass;
		VK_CHECK(vkCreateGraphicsPipelines(g->vk_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &g->vk_graphics_pipeline));

		g->vk_framebuffers = ALLOC_ARRAY(&g->arena_perm, VkFramebuffer, g->vk_swapchain_image_count);
		for (U32 i = 0; i < g->vk_swapchain_image_count; i++) {
			VkFramebufferCreateInfo info = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
			info.renderPass = g->vk_render_pass;
			info.attachmentCount = 1;
			info.pAttachments = &g->vk_swapchain_image_views[i];
			info.width = g->vk_swapchain_info.imageExtent.width;
			info.height = g->vk_swapchain_info.imageExtent.height;
			info.layers = 1;
			VK_CHECK(vkCreateFramebuffer(g->vk_device, &info, nullptr, &g->vk_framebuffers[i]));
		}
	}

	{
		VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		// TODO: Search for graphics queue family index.
		VK_CHECK(vkCreateCommandPool(g->vk_device, &info, nullptr, &g->vk_command_pool));
	}

	// create vertex buffer
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = sizeof(g_vertices);
		buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateBuffer(g->vk_device, &buffer_info, nullptr, &g->vk_vertex_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(g->vk_device, g->vk_vertex_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		mem_info.allocationSize = mem_req.size;
		mem_info.memoryTypeIndex = find_memory_type(g, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK(vkAllocateMemory(g->vk_device, &mem_info, nullptr, &g->vk_vertex_buffer_memory));
		VK_CHECK(vkBindBufferMemory(g->vk_device, g->vk_vertex_buffer, g->vk_vertex_buffer_memory, 0));
	}

	// create staging buffer
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = sizeof(g_vertices);
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateBuffer(g->vk_device, &buffer_info, nullptr, &g->vk_staging_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(g->vk_device, g->vk_staging_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		mem_info.allocationSize = mem_req.size;
		mem_info.memoryTypeIndex = find_memory_type(g, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT| VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK(vkAllocateMemory(g->vk_device, &mem_info, nullptr, &g->vk_staging_buffer_memory));
		VK_CHECK(vkBindBufferMemory(g->vk_device, g->vk_staging_buffer, g->vk_staging_buffer_memory, 0));

		void* data;
		VK_CHECK(vkMapMemory(g->vk_device, g->vk_staging_buffer_memory, 0, buffer_info.size, 0, &data));
		memcpy(data, g_vertices, buffer_info.size);
		vkUnmapMemory(g->vk_device, g->vk_staging_buffer_memory);
	}

	// TODO: Is this too hard-cody of a way to do it?

	{
		VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		info.commandPool = g->vk_command_pool;
		info.commandBufferCount = ARRAY_SIZE(g->vk_command_buffer);
		VK_CHECK(vkAllocateCommandBuffers(g->vk_device, &info, g->vk_command_buffer));
	}

	{
		VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_image_available[0]));
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_render_finished[0]));
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_image_available[1]));
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_render_finished[1]));
	}

	{
		VkFenceCreateInfo info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
		info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		VK_CHECK(vkCreateFence(g->vk_device, &info, nullptr, &g->vk_fence_in_flight[0]));
		VK_CHECK(vkCreateFence(g->vk_device, &info, nullptr, &g->vk_fence_in_flight[1]));
	}

	g->vk_queue = g->vk_queue_families[0].queues[0]; // TODO

	reset(&g->arena_temp);

	return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	Globals* g = static_cast<Globals*>(appstate);

	VK_CHECK(vkWaitForFences(g->vk_device, 1, &g->vk_fence_in_flight[g->vk_current_frame], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(g->vk_device, 1, &g->vk_fence_in_flight[g->vk_current_frame]));

	U32 image_idx;
	VK_CHECK(vkAcquireNextImageKHR(g->vk_device, g->vk_swapchain, UINT64_MAX, g->vk_sem_image_available[g->vk_current_frame], VK_NULL_HANDLE, &image_idx));

	if (g->vk_current_frame == 0) {
		if (g->window_visible == WINDOW_VISIBLE_HIDE) {
			g->window_visible = WINDOW_VISIBLE_SHOW;
		}
		else if (g->window_visible == WINDOW_VISIBLE_SHOW) {
			CHECK(SDL_ShowWindow(g->window));
			g->window_visible = WINDOW_VISIBLE_ALREADY_SHOWN;
		}
	}

	VkCommandBuffer cb = g->vk_command_buffer[g->vk_current_frame];

	{
		VkCommandBufferBeginInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK(vkBeginCommandBuffer(cb, &info));
	}

	if (!g->staged_vertex_buffer) {
		g->staged_vertex_buffer = true;

		VkBufferCopy region = {0, 0, sizeof(g_vertices)};
		vkCmdCopyBuffer(cb, g->vk_staging_buffer, g->vk_vertex_buffer, 1, &region);
	}

	{
		VkClearValue clear_color = { {{0.0f, 0.0f, 0.0f, 1.0f}} };

		VkRenderPassBeginInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
		info.renderPass = g->vk_render_pass;
		info.framebuffer = g->vk_framebuffers[image_idx];
		info.renderArea.extent = g->vk_swapchain_info.imageExtent;
		info.clearValueCount = 1;
		info.pClearValues = &clear_color;
		vkCmdBeginRenderPass(cb, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g->vk_graphics_pipeline);
	vkCmdSetViewport(cb, 0, 1, &g->vk_viewport);
	vkCmdSetScissor(cb, 0, 1, &g->vk_scissor);

	{
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cb, 0, 1, &g->vk_vertex_buffer, &offset);
	}

	vkCmdDraw(cb, ARRAY_SIZE(g_vertices), 1, 0, 0);

	vkCmdEndRenderPass(cb);

	{
		VK_CHECK(vkEndCommandBuffer(cb));

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &g->vk_sem_image_available[g->vk_current_frame];
		submit_info.pWaitDstStageMask = &wait_stage;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &cb;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &g->vk_sem_render_finished[g->vk_current_frame];
		VK_CHECK(vkQueueSubmit(g->vk_queue, 1, &submit_info, g->vk_fence_in_flight[g->vk_current_frame]));

		VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &g->vk_sem_render_finished[g->vk_current_frame];
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &g->vk_swapchain;
		present_info.pImageIndices = &image_idx;
		VK_CHECK(vkQueuePresentKHR(g->vk_queue, &present_info));
	}

	g->vk_current_frame = (g->vk_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

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

void* _alloc(Arena* arena, Int size, Int align) {
	assert(align % 2 == 0);
	Int data = reinterpret_cast<Int>(arena->data) + arena->len;
	Int aligned_data = (data + align - 1) & -align;
	Int final_data = aligned_data + size;
	arena->len += final_data - data;
	assert(arena->len < arena->cap);
	return reinterpret_cast<void*>(aligned_data);
}

void reset(Arena* arena) {
	arena->len = 0;
}

void free(Arena* arena) {
	free(arena->data);
	*arena = {};
}

U32 find_memory_type(Globals* g, U32 memory_types, VkMemoryPropertyFlags properties) {
	for (U32 memory_type = 0; memory_type < g->vk_physical_device_memory_properties.memoryTypeCount; memory_type++) {
		if (HAS_FLAG(memory_types, 1 << memory_type) && HAS_FLAG(g->vk_physical_device_memory_properties.memoryTypes[memory_type].propertyFlags, properties)) {
			return memory_type;
		}
	}
	return 0;
}

/*

Input assembler: pass raw vertex data.
Vertex shader: runs for every vertex.
Tessellation shader: increases mesh quality.
Geometry shader: not used much anymore.
Rasterization stage: Turns primitives (triangles, lines, points) into fragments, AKA pixels.
Fragment shader: Runs for every fragment.
Color blending stage: Blends overlapping fragments into final pixels on the screen.

*/