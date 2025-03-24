#include "main.h"

// TODO
#define SCREEN_WIDTH 1920
#define SCREEN_HEIGHT 1080

#ifdef _DEBUG
VkBool32 VKAPI_CALL vk_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT types, const VkDebugUtilsMessengerCallbackDataEXT* data, void* user_data) {
	SDL_Log(data->pMessage);
	return VK_FALSE;
}
#endif
 
// TODO: Search for supported extensions.
// TODO: remove global variables.

#ifdef _DEBUG
static const char* const g_vk_layers[] = { "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
static const char* const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils" };
static const char* const g_vk_device_extensions[] = { "VK_KHR_swapchain" };
#endif
#ifndef _DEBUG
static const char* const g_vk_instance_extensions[] = { "VK_KHR_surface", "VK_KHR_win32_surface" };
static const char* const g_vk_device_extensions[] = { "VK_KHR_swapchain" };
#endif

static const F32 g_queue_priorities[] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

static const Vertex g_vertices[] = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

static const U16 g_indices[] = {
	0, 1, 2, 2, 3, 0,
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

	CHECK(SDL_GetCurrentTime(&g->start_time));

	SDL_WindowFlags flags = SDL_WINDOW_VULKAN|SDL_WINDOW_HIGH_PIXEL_DENSITY|SDL_WINDOW_HIDDEN;
	g->window = SDL_CreateWindow("GPU Journey", SCREEN_WIDTH, SCREEN_HEIGHT, flags); assert(g->window);

	{
		PFN_vkGetInstanceProcAddr p = reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()); assert(p);
		volkInitializeCustom(p);
	}
	
	// create_instance
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
		create_info.enabledLayerCount = ARRAY_COUNT(g_vk_layers);
		create_info.ppEnabledLayerNames = g_vk_layers;
#endif
		create_info.enabledExtensionCount = ARRAY_COUNT(g_vk_instance_extensions);
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

	// get_physical_device
	{
		// TODO: Query all available physical devices.
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

	// create_device_and_get_device_queues
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
			info.enabledExtensionCount = ARRAY_COUNT(g_vk_device_extensions);
			info.pEnabledFeatures = &physical_device_features;

			VK_CHECK(vkCreateDevice(g->vk_physical_device, &info, nullptr, &g->vk_device));
		}

		volkLoadDevice(g->vk_device);

		g->vk_queue_families = ALLOC_ARRAY(&g->arena_perm, VkQueueFamily, g->vk_queue_family_count);
		for (U32 queue_family_idx = 0; queue_family_idx < g->vk_queue_family_count; queue_family_idx++) {
			VkQueueFamily* queue_family = &g->vk_queue_families[queue_family_idx];
			queue_family->properties = queue_family_properties[queue_family_idx];
			queue_family->queues = ALLOC_ARRAY(&g->arena_perm, VkQueue, queue_family->queue_count);
			for (U32 queue_idx = 0; queue_idx < queue_family->queue_count; queue_idx++) {
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

	// create_shader_module
	{
		size_t len;
		void* data = SDL_LoadFile("vert.spv", &len); assert(data);

		VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		info.codeSize = len;
		info.pCode = reinterpret_cast<U32*>(data);
		VK_CHECK(vkCreateShaderModule(g->vk_device, &info, nullptr, &g->vk_vs_module));
	}

	// create_shader_module
	{
		size_t len;
		void* data = SDL_LoadFile("frag.spv", &len); assert(data);

		VkShaderModuleCreateInfo info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
		info.codeSize = len;
		info.pCode = reinterpret_cast<U32*>(data);
		VK_CHECK(vkCreateShaderModule(g->vk_device, &info, nullptr, &g->vk_fs_module));
	}

	// create_descriptor_set_layout
	{
		VkDescriptorSetLayoutBinding b_uniform_buffer = {};
		b_uniform_buffer.binding = 0;
		b_uniform_buffer.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		b_uniform_buffer.descriptorCount = 1;
		b_uniform_buffer.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

		VkDescriptorSetLayoutBinding b_sampler = {};
		b_sampler.binding = 1;
		b_sampler.descriptorCount = 1;
		b_sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		b_sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		VkDescriptorSetLayoutBinding bindings[] = { b_uniform_buffer, b_sampler };

		VkDescriptorSetLayoutCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		info.bindingCount = ARRAY_COUNT(bindings);
		info.pBindings = bindings;
		VK_CHECK(vkCreateDescriptorSetLayout(g->vk_device, &info, nullptr, &g->vk_descriptor_set_layout));
	}

	// create_pipeline_layout
	{
		VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
		pipeline_layout_info.setLayoutCount = 1;
		pipeline_layout_info.pSetLayouts = &g->vk_descriptor_set_layout;
		VK_CHECK(vkCreatePipelineLayout(g->vk_device, &pipeline_layout_info, nullptr, &g->vk_pipeline_layout));
	}

	// create_render_pass
	{
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

		VkRenderPassCreateInfo info = { VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO };
		info.attachmentCount = 1;
		info.pAttachments = &color_attachment;
		info.subpassCount = 1;
		info.pSubpasses = &subpass;
		info.dependencyCount = 1;
		info.pDependencies = &subpass_dependency;
		VK_CHECK(vkCreateRenderPass(g->vk_device, &info, nullptr, &g->vk_render_pass));
	}

	// create_graphics_pipeline
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
		dynamic_state_info.dynamicStateCount = ARRAY_COUNT(dynamic_states);
		dynamic_state_info.pDynamicStates = dynamic_states;

		VkVertexInputBindingDescription vertex_input_binding = {};
		vertex_input_binding.binding = 0;
		vertex_input_binding.stride = sizeof(Vertex);
		vertex_input_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		VkVertexInputAttributeDescription vertex_attributes[3] = {};

		vertex_attributes[0].binding = 0;
		vertex_attributes[0].location = 0;
		vertex_attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attributes[0].offset = offsetof(Vertex, pos);

		vertex_attributes[1].binding = 0;
		vertex_attributes[1].location = 1;
		vertex_attributes[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		vertex_attributes[1].offset = offsetof(Vertex, color);

		vertex_attributes[2].binding = 0;
		vertex_attributes[2].location = 2;
		vertex_attributes[2].format = VK_FORMAT_R32G32_SFLOAT;
		vertex_attributes[2].offset = offsetof(Vertex, tex_coord);

		VkPipelineVertexInputStateCreateInfo vertex_input_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
		vertex_input_info.vertexBindingDescriptionCount = 1;
		vertex_input_info.pVertexBindingDescriptions = &vertex_input_binding;
		vertex_input_info.vertexAttributeDescriptionCount = ARRAY_COUNT(vertex_attributes);
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
	}

	// create_framebuffers
	{
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

	// create_command_pool
	{
		VkCommandPoolCreateInfo info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		// TODO: Search for graphics queue family index.
		VK_CHECK(vkCreateCommandPool(g->vk_device, &info, nullptr, &g->vk_command_pool));
	}

	// create_image
	{
		SDL_Surface* raw_surf = IMG_Load("texture.jpg");
		g->surf = SDL_ConvertSurface(raw_surf, SDL_PIXELFORMAT_RGBA32);
		SDL_DestroySurface(raw_surf);

		g->vk_image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		g->vk_image_info.imageType = VK_IMAGE_TYPE_2D;
		g->vk_image_info.extent.width = static_cast<uint32_t>(g->surf->w);
		g->vk_image_info.extent.height = static_cast<uint32_t>(g->surf->h);
		g->vk_image_info.extent.depth = 1;
		g->vk_image_info.mipLevels = 1;
		g->vk_image_info.arrayLayers = 1;
		g->vk_image_info.format = VK_FORMAT_R8G8B8A8_UNORM;
		g->vk_image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		g->vk_image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		g->vk_image_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		g->vk_image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		g->vk_image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		VK_CHECK(vkCreateImage(g->vk_device, &g->vk_image_info, nullptr, &g->vk_image));

		VkMemoryRequirements mem_req;
		vkGetImageMemoryRequirements(g->vk_device, g->vk_image, &mem_req);

		VkMemoryAllocateInfo mem_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		mem_info.allocationSize = mem_req.size;
		mem_info.memoryTypeIndex = find_memory_type(g, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK(vkAllocateMemory(g->vk_device, &mem_info, nullptr, &g->vk_image_memory));
		VK_CHECK(vkBindImageMemory(g->vk_device, g->vk_image, g->vk_image_memory, 0));
	}

	// create_image_view
	{
		VkImageViewCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
		info.image = g->vk_image;
		info.viewType = VK_IMAGE_VIEW_TYPE_2D;
		info.format = VK_FORMAT_R8G8B8A8_UNORM;
		info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		info.subresourceRange.levelCount = 1;
		info.subresourceRange.layerCount = 1;
		VK_CHECK(vkCreateImageView(g->vk_device, &info, nullptr, &g->vk_image_view));
	}

	// create_sampler
	{
		VkSamplerCreateInfo info = { VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO };
		info.magFilter = VK_FILTER_LINEAR;
		info.minFilter = VK_FILTER_LINEAR;
		info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		info.anisotropyEnable = VK_TRUE;
		info.maxAnisotropy = g->vk_physical_device_properties.limits.maxSamplerAnisotropy;
		info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		info.compareOp = VK_COMPARE_OP_ALWAYS;
		info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		VK_CHECK(vkCreateSampler(g->vk_device, &info, nullptr, &g->vk_sampler));
	}

	// create_vertex_buffer
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

	// create_index_buffer
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = sizeof(g_indices);
		buffer_info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateBuffer(g->vk_device, &buffer_info, nullptr, &g->vk_index_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(g->vk_device, g->vk_index_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		mem_info.allocationSize = mem_req.size;
		mem_info.memoryTypeIndex = find_memory_type(g, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		VK_CHECK(vkAllocateMemory(g->vk_device, &mem_info, nullptr, &g->vk_index_buffer_memory));
		VK_CHECK(vkBindBufferMemory(g->vk_device, g->vk_index_buffer, g->vk_index_buffer_memory, 0));
	}

	// create_staging_buffer
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = sizeof(g_vertices) + sizeof(g_indices);
		buffer_info.size += g->surf->w * g->surf->h * 4;
		buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateBuffer(g->vk_device, &buffer_info, nullptr, &g->vk_staging_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(g->vk_device, g->vk_staging_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		mem_info.allocationSize = mem_req.size;
		mem_info.memoryTypeIndex = find_memory_type(g, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK(vkAllocateMemory(g->vk_device, &mem_info, nullptr, &g->vk_staging_buffer_memory));
		VK_CHECK(vkBindBufferMemory(g->vk_device, g->vk_staging_buffer, g->vk_staging_buffer_memory, 0));

		void* data;
		VK_CHECK(vkMapMemory(g->vk_device, g->vk_staging_buffer_memory, 0, buffer_info.size, 0, &data));
		memcpy(data, g_vertices, sizeof(g_vertices));
		UInt offset = reinterpret_cast<UInt>(data) + sizeof(g_vertices);
		memcpy(reinterpret_cast<void*>(offset), g_indices, sizeof(g_indices));
		offset += sizeof(g_indices);
		memcpy(reinterpret_cast<void*>(offset), g->surf->pixels, g->surf->w* g->surf->h * 4);
		vkUnmapMemory(g->vk_device, g->vk_staging_buffer_memory);
	}

	// create_uniform_buffer
	{
		VkBufferCreateInfo buffer_info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		buffer_info.size = sizeof(Uniform_Buffer_Object) * 2;
		buffer_info.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
		buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		VK_CHECK(vkCreateBuffer(g->vk_device, &buffer_info, nullptr, &g->vk_uniform_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(g->vk_device, g->vk_uniform_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info = { VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO };
		mem_info.allocationSize = mem_req.size;
		mem_info.memoryTypeIndex = find_memory_type(g, mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

		VK_CHECK(vkAllocateMemory(g->vk_device, &mem_info, nullptr, &g->vk_uniform_buffer_memory));
		VK_CHECK(vkBindBufferMemory(g->vk_device, g->vk_uniform_buffer, g->vk_uniform_buffer_memory, 0));

		VK_CHECK(vkMapMemory(g->vk_device, g->vk_uniform_buffer_memory, 0, mem_req.size, 0, &g->vk_uniform_buffer_mapped));
	}

	// create_descriptor_pool
	{
		VkDescriptorPoolSize sizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT },
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT },
		};

		VkDescriptorPoolCreateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
		info.poolSizeCount = ARRAY_COUNT(sizes); 
		info.pPoolSizes = sizes;
		info.maxSets = MAX_FRAMES_IN_FLIGHT;
		VK_CHECK(vkCreateDescriptorPool(g->vk_device, &info, nullptr, &g->vk_descriptor_pool));
	}

	// create_descriptor_sets
	{
		VkDescriptorSetAllocateInfo info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
		info.descriptorPool = g->vk_descriptor_pool;
		info.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		VkDescriptorSetLayout layouts[] = { g->vk_descriptor_set_layout, g->vk_descriptor_set_layout };
		info.pSetLayouts = layouts;
		VK_CHECK(vkAllocateDescriptorSets(g->vk_device, &info, g->vk_descriptor_sets));
	}

	// update_descriptor_sets
	{
		VkDescriptorBufferInfo buffer_infos[2] = {};
		VkDescriptorImageInfo image_info = {};
		VkWriteDescriptorSet write_infos[4] = {};

		buffer_infos[0].buffer = g->vk_uniform_buffer;
		buffer_infos[0].offset = 0;
		buffer_infos[0].range = sizeof(Uniform_Buffer_Object);

		buffer_infos[1].buffer = g->vk_uniform_buffer;
		buffer_infos[1].offset = sizeof(Uniform_Buffer_Object);
		buffer_infos[1].range = sizeof(Uniform_Buffer_Object);

		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = g->vk_image_view;
		image_info.sampler = g->vk_sampler;

		write_infos[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[0].dstSet = g->vk_descriptor_sets[0];
		write_infos[0].dstBinding = 0;
		write_infos[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_infos[0].descriptorCount = 1;
		write_infos[0].pBufferInfo = &buffer_infos[0];

		write_infos[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[1].dstSet = g->vk_descriptor_sets[1];
		write_infos[1].dstBinding = 0;
		write_infos[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_infos[1].descriptorCount = 1;
		write_infos[1].pBufferInfo = &buffer_infos[1];

		write_infos[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[2].dstSet = g->vk_descriptor_sets[0];
		write_infos[2].dstBinding = 1;
		write_infos[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_infos[2].descriptorCount = 1;
		write_infos[2].pImageInfo = &image_info;

		write_infos[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[3].dstSet = g->vk_descriptor_sets[1];
		write_infos[3].dstBinding = 1;
		write_infos[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_infos[3].descriptorCount = 1;
		write_infos[3].pImageInfo = &image_info;
		
		vkUpdateDescriptorSets(g->vk_device, ARRAY_COUNT(write_infos), write_infos, 0, nullptr);
	}

	// allocate_command_buffers
	{
		VkCommandBufferAllocateInfo info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
		info.commandPool = g->vk_command_pool;
		info.commandBufferCount = ARRAY_COUNT(g->vk_command_buffer);
		VK_CHECK(vkAllocateCommandBuffers(g->vk_device, &info, g->vk_command_buffer));
	}

	// create_semaphores
	{
		VkSemaphoreCreateInfo info = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_image_available[0]));
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_render_finished[0]));
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_image_available[1]));
		VK_CHECK(vkCreateSemaphore(g->vk_device, &info, nullptr, &g->vk_sem_render_finished[1]));
	}

	// create_fences
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

	if (!g->vk_staged) {
		g->vk_staged= true;

		{
			VkBufferCopy region = { 0, 0, sizeof(g_vertices) };
			vkCmdCopyBuffer(cb, g->vk_staging_buffer, g->vk_vertex_buffer, 1, &region);
		}
		{
			VkBufferCopy region = { sizeof(g_vertices), 0, sizeof(g_indices)};
			vkCmdCopyBuffer(cb, g->vk_staging_buffer, g->vk_index_buffer, 1, &region);
		}

		{
			VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = g->vk_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
		{
			VkBufferImageCopy region = {};
			region.bufferOffset = sizeof(g_vertices) + sizeof(g_indices);
			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.layerCount = 1;
			region.imageExtent = g->vk_image_info.extent;
			vkCmdCopyBufferToImage(cb, g->vk_staging_buffer, g->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}
		{
			VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = g->vk_image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
	}
	else {
		VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = g->vk_image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.layerCount = 1;
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
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

	vkCmdBindIndexBuffer(cb, g->vk_index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, g->vk_pipeline_layout, 0, 1, &g->vk_descriptor_sets[g->vk_current_frame], 0, nullptr);
	vkCmdDrawIndexed(cb, ARRAY_COUNT(g_indices), 1, 0, 0, 0);

	vkCmdEndRenderPass(cb); 

	// update_uniform_buffer
	{
		I64 current_time;
		CHECK(SDL_GetCurrentTime(&current_time));
		const F32 NS_PER_SECOND = 1'000'000'000.0f;
		F32 time = static_cast<F32>(current_time - g->start_time) / NS_PER_SECOND;

		Uniform_Buffer_Object ubo = {};
		ubo.model = HMM_Rotate_RH(time * HMM_AngleDeg(90.0f), HMM_Vec3{ 1.0f, 0.0f, 1.0f });
		ubo.view = HMM_LookAt_RH(HMM_Vec3{ 2.0f, 2.0f, 2.0f }, HMM_Vec3{ 0.0f, 0.0f, 0.0f }, HMM_Vec3{ 0.0f, 0.0f, 1.0f });
		ubo.proj = HMM_Perspective_RH_NO(HMM_AngleDeg(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
		UInt dest = reinterpret_cast<UInt>(g->vk_uniform_buffer_mapped) + g->vk_current_frame * sizeof(Uniform_Buffer_Object);
		memcpy(reinterpret_cast<void*>(dest), &ubo, sizeof(ubo));
	}

	// submit_next_frame 
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

	reset(&g->arena_temp);

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