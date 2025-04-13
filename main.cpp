#include "main.h"

#pragma warning(push, 4)
#pragma warning(disable:4100)
#pragma warning(disable:4189)

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
static std::array<const char* const, 2> g_vk_layers{ "VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor" };
static std::array<const char* const, 3> g_vk_instance_extensions{ "VK_KHR_surface", "VK_KHR_win32_surface", "VK_EXT_debug_utils"};
static std::array<const char* const, 1> g_vk_device_extensions{ "VK_KHR_swapchain" };
#endif
#ifndef _DEBUG
static std::array<const char* const, 2> g_vk_instance_extensions{ "VK_KHR_surface", "VK_KHR_win32_surface" };
static std::array<const char* const, 1> g_vk_device_extensions{ "VK_KHR_swapchain" };
#endif

static std::array<const float, 16> g_queue_priorities{ 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

static const std::array<Vertex, 8> g_vertices{
	Vertex
	{{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f,  0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	{{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}}
};

static std::array<const uint16_t, 12> g_indices{ 0, 1, 2, 2, 3, 0, 4, 5, 6, 6, 7, 4 };

#define VK_CHECK(EXPR) STMT(VkResult res = EXPR; SDL_assert(res == VK_SUCCESS);)
#define SDL_CHECK(EXPR) STMT(if (!EXPR) { SDL_Log(SDL_GetError()); return SDL_APP_FAILURE; })

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv) {
	SDL_CHECK(SDL_Init(SDL_INIT_VIDEO));

	Context* ctx;
	{
		void* memory = SDL_malloc(MEGABYTE(16));
		SDL_CHECK(memory);
		Arena arena_perm{memory, MEGABYTE(8)};
		Arena arena_temp{reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(memory) + MEGABYTE(8)), MEGABYTE(8)};
		ctx = arena_perm.alloc<Context>();
		*ctx = {};
		*appstate = ctx;
		ctx->arena_perm = arena_perm;
		ctx->arena_temp = arena_temp;
	}

	SDL_CHECK(SDL_GetCurrentTime(&ctx->start_time));

	SDL_WindowFlags flags{ SDL_WINDOW_VULKAN | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN };
	ctx->window = SDL_CreateWindow("GPU Journey", SCREEN_WIDTH, SCREEN_HEIGHT, flags); SDL_CHECK(ctx->window);

	{
		PFN_vkGetInstanceProcAddr p{ reinterpret_cast<PFN_vkGetInstanceProcAddr>(SDL_Vulkan_GetVkGetInstanceProcAddr()) }; SDL_CHECK(p);
		volkInitializeCustom(p);
	}
	
	// create_instance
	{
		VkApplicationInfo app_info{ 
			.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
			.pApplicationName = "GPU Journey",
			.applicationVersion = VK_API_VERSION_1_0,
			.pEngineName = "GPU Journey",
			.engineVersion = VK_API_VERSION_1_0,
			.apiVersion = VK_API_VERSION_1_0,
		};

		VkInstanceCreateInfo create_info{ 
			.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
			.pApplicationInfo = &app_info,
#ifdef _DEBUG
			.enabledLayerCount = static_cast<uint32_t>(g_vk_layers.size()),
			.ppEnabledLayerNames = g_vk_layers.data(),
#endif
			.enabledExtensionCount = static_cast<uint32_t>(g_vk_instance_extensions.size()),
			.ppEnabledExtensionNames = g_vk_instance_extensions.data(),
		};

#ifdef _DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debug_info{ 
			.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
			.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT,
			.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
			.pfnUserCallback = vk_debug_callback,
			.pUserData = ctx,
		};

		create_info.pNext = &debug_info;
#endif

		VK_CHECK(vkCreateInstance(&create_info, nullptr, &ctx->vk_instance));
	}
	
	volkLoadInstanceOnly(ctx->vk_instance);

	// get_physical_device
	{
		// TODO: Query all available physical devices.
		uint32_t physical_device_count = 1;
		VK_CHECK(vkEnumeratePhysicalDevices(ctx->vk_instance, &physical_device_count, &ctx->vk_physical_device));
		vkGetPhysicalDeviceProperties(ctx->vk_physical_device, &ctx->vk_physical_device_properties);
		vkGetPhysicalDeviceMemoryProperties(ctx->vk_physical_device, &ctx->vk_physical_device_memory_properties);
	}

	SDL_CHECK(SDL_Vulkan_CreateSurface(ctx->window, ctx->vk_instance, nullptr, &ctx->vk_surface));

	VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(ctx->vk_physical_device, ctx->vk_surface, &ctx->vk_surface_capabilities));

	// get_physical_device_surface_formats
	{
		uint32_t count;
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, ctx->vk_surface, &count, nullptr));
		ctx->vk_surface_formats = ctx->arena_perm.alloc_span<VkSurfaceFormatKHR>(count);
		VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(ctx->vk_physical_device, ctx->vk_surface, &count, ctx->vk_surface_formats.data()));
	}

	// create_device_and_get_device_queues
	{
		{
			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(ctx->vk_physical_device, &count, nullptr);
			ctx->vk_queue_family_properties = ctx->arena_temp.alloc_span<VkQueueFamilyProperties>(count);
			vkGetPhysicalDeviceQueueFamilyProperties(ctx->vk_physical_device, &count, ctx->vk_queue_family_properties.data());
		}

		VkPhysicalDeviceFeatures physical_device_features;
		vkGetPhysicalDeviceFeatures(ctx->vk_physical_device, &physical_device_features);

		{
			std::span<VkDeviceQueueCreateInfo> queue_infos = ctx->arena_temp.alloc_span<VkDeviceQueueCreateInfo>(ctx->vk_queue_family_properties.size());
			size_t queue_info_count = 0;
			for (size_t queue_family_idx = 0; queue_family_idx < ctx->vk_queue_family_properties.size(); queue_family_idx += 1) {
				if (ctx->vk_queue_family_properties[queue_family_idx].queueCount == 0) continue;
				queue_infos[queue_info_count] = {
					.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
					.queueFamilyIndex = static_cast<uint32_t>(queue_family_idx),
					.queueCount = ctx->vk_queue_family_properties[queue_family_idx].queueCount,
					.pQueuePriorities = g_queue_priorities.data(),
				};
				queue_info_count += 1;
			}

			VkDeviceCreateInfo info{ 
				.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
				.queueCreateInfoCount = static_cast<uint32_t>(queue_info_count),
				.pQueueCreateInfos = queue_infos.data(),
				.enabledExtensionCount = static_cast<uint32_t>(g_vk_device_extensions.size()),
				.ppEnabledExtensionNames = g_vk_device_extensions.data(),
				.pEnabledFeatures = &physical_device_features,
			};
			VK_CHECK(vkCreateDevice(ctx->vk_physical_device, &info, nullptr, &ctx->vk_device));
		}

		volkLoadDevice(ctx->vk_device);

		ctx->vk_queues = ctx->arena_perm.alloc_span<std::span<VkQueue>>(ctx->vk_queue_family_properties.size());
		for (size_t queue_family_idx = 0; queue_family_idx < ctx->vk_queue_family_properties.size(); queue_family_idx += 1) {
			ctx->vk_queues[queue_family_idx] = ctx->arena_perm.alloc_span<VkQueue>(ctx->vk_queue_family_properties[queue_family_idx].queueCount);
			for (uint32_t queue_idx = 0; queue_idx < ctx->vk_queue_family_properties[queue_family_idx].queueCount; queue_idx++) {
				vkGetDeviceQueue(ctx->vk_device, static_cast<uint32_t>(queue_family_idx), queue_idx, ctx->vk_queues[queue_family_idx].data());
			}
		}
	}

	// create_swapchain
	{
		ctx->vk_swapchain_info = { 
			.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
			.surface = ctx->vk_surface,
			.minImageCount = glm::min(ctx->vk_surface_capabilities.minImageCount + 1, ctx->vk_surface_capabilities.maxImageCount),
			.imageFormat = VK_FORMAT_R8G8B8A8_UNORM, // TODO
			.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR, // TODO
			.imageExtent = ctx->vk_surface_capabilities.currentExtent,
			.imageArrayLayers = 1,
			.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			.preTransform = ctx->vk_surface_capabilities.currentTransform,
			.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
			.presentMode = VK_PRESENT_MODE_FIFO_KHR,
			.clipped = VK_TRUE,
		};
		
		VK_CHECK(vkCreateSwapchainKHR(ctx->vk_device, &ctx->vk_swapchain_info, nullptr, &ctx->vk_swapchain));
	}

	// get_swapchain_images
	{
		uint32_t swapchain_image_count;
		VK_CHECK(vkGetSwapchainImagesKHR(ctx->vk_device, ctx->vk_swapchain, &swapchain_image_count, nullptr));
		ctx->vk_swapchain_images = ctx->arena_perm.alloc_span<VkImage>(swapchain_image_count);
		VK_CHECK(vkGetSwapchainImagesKHR(ctx->vk_device, ctx->vk_swapchain, &swapchain_image_count, ctx->vk_swapchain_images.data()));
	}

	// create_swapchain_image_view
	{
		ctx->vk_swapchain_image_views = ctx->arena_perm.alloc_span<VkImageView>(ctx->vk_swapchain_images.size());
		for (size_t swapchain_image_idx = 0; swapchain_image_idx < ctx->vk_swapchain_images.size(); swapchain_image_idx++) {
			VkImageViewCreateInfo info{ 
				.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
				.image = ctx->vk_swapchain_images[swapchain_image_idx],
				.viewType = VK_IMAGE_VIEW_TYPE_2D,
				.format = ctx->vk_swapchain_info.imageFormat,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
			};
			VK_CHECK(vkCreateImageView(ctx->vk_device, &info, nullptr, &ctx->vk_swapchain_image_views[swapchain_image_idx]));
		}
	}

	// create_shader_module
	{
		size_t len;
		void* data = SDL_LoadFile("vert.spv", &len); SDL_assert(data);

		VkShaderModuleCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = len,
			.pCode = reinterpret_cast<uint32_t*>(data),
		};
		VK_CHECK(vkCreateShaderModule(ctx->vk_device, &info, nullptr, &ctx->vk_vs_module));

		SDL_free(data);
	}

	// create_shader_module
	{
		size_t len;
		void* data = SDL_LoadFile("frag.spv", &len); SDL_assert(data);

		VkShaderModuleCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = len,
			.pCode = reinterpret_cast<uint32_t*>(data),
		};
		VK_CHECK(vkCreateShaderModule(ctx->vk_device, &info, nullptr, &ctx->vk_fs_module));

		SDL_free(data);
	}

	// create_descriptor_set_layout
	{
		VkDescriptorSetLayoutBinding b_uniform_buffer{
			.binding = 0,
			.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
		};

		VkDescriptorSetLayoutBinding b_sampler{
			.binding = 1,
			.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = 1,
			.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
		};

		std::array<VkDescriptorSetLayoutBinding, 2> bindings{ b_uniform_buffer, b_sampler };

		VkDescriptorSetLayoutCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = static_cast<uint32_t>(bindings.size()),
			.pBindings = bindings.data(),
		};
		
		VK_CHECK(vkCreateDescriptorSetLayout(ctx->vk_device, &info, nullptr, &ctx->vk_descriptor_set_layout));
	}

	// create_pipeline_layout
	{
		VkPipelineLayoutCreateInfo pipeline_layout_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
			.setLayoutCount = 1,
			.pSetLayouts = &ctx->vk_descriptor_set_layout,
		};

		VK_CHECK(vkCreatePipelineLayout(ctx->vk_device, &pipeline_layout_info, nullptr, &ctx->vk_pipeline_layout));
	}

	// create_render_pass
	{
		VkAttachmentDescription color_attachment{
			.format = ctx->vk_swapchain_info.imageFormat,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
		};

		VkAttachmentReference color_attachment_ref{
			.attachment = 0, // index in attachments array
			.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentDescription depth_attachment{
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
			.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
			.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkAttachmentReference depth_attachment_ref{
			.attachment = 1,
			.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		};

		VkSubpassDescription subpass{
			.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
			.colorAttachmentCount = 1,
			.pColorAttachments = &color_attachment_ref,
			.pDepthStencilAttachment = &depth_attachment_ref,
		};

		VkSubpassDependency subpass_dependency{
			.srcSubpass = VK_SUBPASS_EXTERNAL, // implicit subpass before and after
			.dstSubpass = 0, // subpass index
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		};

		std::array<VkAttachmentDescription, 2> attachments{ color_attachment, depth_attachment };

		VkRenderPassCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
			.attachmentCount = static_cast<uint32_t>(attachments.size()),
			.pAttachments = attachments.data(),
			.subpassCount = 1,
			.pSubpasses = &subpass,
			.dependencyCount = 1,
			.pDependencies = &subpass_dependency,
		};

		VK_CHECK(vkCreateRenderPass(ctx->vk_device, &info, nullptr, &ctx->vk_render_pass));
	}

	// create_graphics_pipeline
	{
		VkPipelineShaderStageCreateInfo vert_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_VERTEX_BIT,
			.module = ctx->vk_vs_module,
			.pName = "main",
		};
		
		VkPipelineShaderStageCreateInfo frag_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
			.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
			.module = ctx->vk_fs_module,
			.pName = "main",
		};
		
		std::array<VkPipelineShaderStageCreateInfo, 2> shader_stages{
			vert_info,
			frag_info,
		};

		std::array<VkDynamicState, 2> dynamic_states{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR,
		};

		VkPipelineDynamicStateCreateInfo dynamic_state_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
			.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
			.pDynamicStates = dynamic_states.data(),
		};
		
		VkVertexInputBindingDescription vertex_input_binding{
			.binding = 0,
			.stride = sizeof(Vertex),
			.inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
		};
		
		std::array<VkVertexInputAttributeDescription, 3> vertex_attributes{
			VkVertexInputAttributeDescription{
				.location = 0,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, pos),
			},
			{
				.location = 1,
				.binding = 0,
				.format = VK_FORMAT_R32G32B32_SFLOAT,
				.offset = offsetof(Vertex, color),
			},
			{
				.location = 2,
				.binding = 0,
				.format = VK_FORMAT_R32G32_SFLOAT,
				.offset = offsetof(Vertex, tex_coord),
			},
		};

		VkPipelineVertexInputStateCreateInfo vertex_input_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
			.vertexBindingDescriptionCount = 1,
			.pVertexBindingDescriptions = &vertex_input_binding,
			.vertexAttributeDescriptionCount = static_cast<uint32_t>(vertex_attributes.size()),
			.pVertexAttributeDescriptions = vertex_attributes.data(),
		};
		
		// TODO: what does primitive restart do?
		VkPipelineInputAssemblyStateCreateInfo input_assembly_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
			.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		};
		
		ctx->vk_viewport = {
			.width = static_cast<float>(ctx->vk_swapchain_info.imageExtent.width),
			.height = static_cast<float>(ctx->vk_swapchain_info.imageExtent.height),
			.maxDepth = 1.0f,
		};
		
		ctx->vk_scissor.extent = ctx->vk_swapchain_info.imageExtent;

		VkPipelineViewportStateCreateInfo viewport_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
			.viewportCount = 1,
			.scissorCount = 1,
		};
		
		VkPipelineRasterizationStateCreateInfo rasterization_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
			.cullMode = VK_CULL_MODE_BACK_BIT,
			.frontFace = VK_FRONT_FACE_CLOCKWISE,
			.lineWidth = 1.0f
		};
		
		VkPipelineMultisampleStateCreateInfo multisample_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
			.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		};

		VkPipelineColorBlendAttachmentState blend_attachment_info{ 
			.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
		};

		VkPipelineDepthStencilStateCreateInfo depth_stencil_info{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
			.depthTestEnable = VK_TRUE,
			.depthWriteEnable = VK_TRUE,
			.depthCompareOp = VK_COMPARE_OP_LESS,

		};

		VkPipelineColorBlendStateCreateInfo blend_info{ 
			.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
			.attachmentCount = 1,
			.pAttachments = &blend_attachment_info,
		};
		
		VkGraphicsPipelineCreateInfo graphics_pipeline_info{ 
			.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
			.stageCount = static_cast<uint32_t>(shader_stages.size()),
			.pStages = shader_stages.data(),
			.pVertexInputState = &vertex_input_info,
			.pInputAssemblyState = &input_assembly_info,
			.pViewportState = &viewport_info,
			.pRasterizationState = &rasterization_info,
			.pMultisampleState = &multisample_info,
			.pDepthStencilState = &depth_stencil_info,
			.pColorBlendState = &blend_info,
			.pDynamicState = &dynamic_state_info,
			.layout = ctx->vk_pipeline_layout,
			.renderPass = ctx->vk_render_pass,
		};
		VK_CHECK(vkCreateGraphicsPipelines(ctx->vk_device, VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &ctx->vk_graphics_pipeline));
	}

	// create_command_pool
	{
		VkCommandPoolCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
		};
	
		// TODO: Search for graphics queue family index.
		VK_CHECK(vkCreateCommandPool(ctx->vk_device, &info, nullptr, &ctx->vk_command_pool));
	}

	// create_depth_stencil_image
	{
		VkImageCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT, // TODO
			.extent = {.width = ctx->vk_swapchain_info.imageExtent.width, .height = ctx->vk_swapchain_info.imageExtent.height, .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};
		VK_CHECK(vkCreateImage(ctx->vk_device, &info, nullptr, &ctx->vk_depth_stencil_image));
	}

	// create_depth_stencil_image_memory
	{
		VkMemoryRequirements mem_req;
		vkGetImageMemoryRequirements(ctx->vk_device, ctx->vk_depth_stencil_image, &mem_req);

		VkMemoryAllocateInfo mem_info{
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.size,
			.memoryTypeIndex = ctx->find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};

		VK_CHECK(vkAllocateMemory(ctx->vk_device, &mem_info, nullptr, &ctx->vk_depth_stencil_image_memory));
		VK_CHECK(vkBindImageMemory(ctx->vk_device, ctx->vk_depth_stencil_image, ctx->vk_depth_stencil_image_memory, 0));
	}

	// create_depth_stencil_image_view
	{
		VkImageViewCreateInfo info{
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = ctx->vk_depth_stencil_image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_D32_SFLOAT_S8_UINT,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT, .levelCount = 1, .layerCount = 1 },
		};

		VK_CHECK(vkCreateImageView(ctx->vk_device, &info, nullptr, &ctx->vk_depth_stencil_image_view));
	}

	// create_framebuffers
	{
		ctx->vk_framebuffers = ctx->arena_perm.alloc_span<VkFramebuffer>(ctx->vk_swapchain_images.size());
		for (size_t i = 0; i < ctx->vk_swapchain_images.size(); i += 1) {
			std::array<VkImageView, 2> attachments{
				ctx->vk_swapchain_image_views[i],
				ctx->vk_depth_stencil_image_view,
			};

			VkFramebufferCreateInfo info{
				.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
				.renderPass = ctx->vk_render_pass,
				.attachmentCount = static_cast<uint32_t>(attachments.size()),
				.pAttachments = attachments.data(),
				.width = ctx->vk_swapchain_info.imageExtent.width,
				.height = ctx->vk_swapchain_info.imageExtent.height,
				.layers = 1,
			};
			VK_CHECK(vkCreateFramebuffer(ctx->vk_device, &info, nullptr, &ctx->vk_framebuffers[i]));
		}
	}
	
	// create_texture_image
	{
		ctx->surf = IMG_Load("viking_room.png");

		ctx->vk_image_info = {
			.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
			.imageType = VK_IMAGE_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.extent = { .width = static_cast<uint32_t>(ctx->surf->w), .height = static_cast<uint32_t>(ctx->surf->h), .depth = 1 },
			.mipLevels = 1,
			.arrayLayers = 1,
			.samples = VK_SAMPLE_COUNT_1_BIT,
			.tiling = VK_IMAGE_TILING_OPTIMAL,
			.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
			.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		};

		VK_CHECK(vkCreateImage(ctx->vk_device, &ctx->vk_image_info, nullptr, &ctx->vk_image));

		VkMemoryRequirements mem_req;
		vkGetImageMemoryRequirements(ctx->vk_device, ctx->vk_image, &mem_req);

		VkMemoryAllocateInfo mem_info{ 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.size,
			.memoryTypeIndex = ctx->find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		
		VK_CHECK(vkAllocateMemory(ctx->vk_device, &mem_info, nullptr, &ctx->vk_image_memory));
		VK_CHECK(vkBindImageMemory(ctx->vk_device, ctx->vk_image, ctx->vk_image_memory, 0));
	}

	// create_image_view
	{
		VkImageViewCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
			.image = ctx->vk_image,
			.viewType = VK_IMAGE_VIEW_TYPE_2D,
			.format = VK_FORMAT_R8G8B8A8_UNORM,
			.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
		};

		VK_CHECK(vkCreateImageView(ctx->vk_device, &info, nullptr, &ctx->vk_image_view));
	}

	// create_sampler
	{
		VkSamplerCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
			.magFilter = VK_FILTER_LINEAR,
			.minFilter = VK_FILTER_LINEAR,
			.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
			.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT,
			.anisotropyEnable = VK_TRUE,
			.maxAnisotropy = ctx->vk_physical_device_properties.limits.maxSamplerAnisotropy,
			.compareOp = VK_COMPARE_OP_ALWAYS,
			.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		};
		VK_CHECK(vkCreateSampler(ctx->vk_device, &info, nullptr, &ctx->vk_sampler));
	}

	// create_vertex_buffer
	{
		VkBufferCreateInfo buffer_info{ 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(g_vertices),
			.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};
		
		VK_CHECK(vkCreateBuffer(ctx->vk_device, &buffer_info, nullptr, &ctx->vk_vertex_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(ctx->vk_device, ctx->vk_vertex_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info{ 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.size,
			.memoryTypeIndex = ctx->find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		
		VK_CHECK(vkAllocateMemory(ctx->vk_device, &mem_info, nullptr, &ctx->vk_vertex_buffer_memory));
		VK_CHECK(vkBindBufferMemory(ctx->vk_device, ctx->vk_vertex_buffer, ctx->vk_vertex_buffer_memory, 0));
	}

	// create_index_buffer
	{
		VkBufferCreateInfo buffer_info{ 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(g_indices),
			.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};
		
		VK_CHECK(vkCreateBuffer(ctx->vk_device, &buffer_info, nullptr, &ctx->vk_index_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(ctx->vk_device, ctx->vk_index_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info{ 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO, 
			.allocationSize = mem_req.size,
			.memoryTypeIndex = ctx->find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT),
		};
		
		VK_CHECK(vkAllocateMemory(ctx->vk_device, &mem_info, nullptr, &ctx->vk_index_buffer_memory));
		VK_CHECK(vkBindBufferMemory(ctx->vk_device, ctx->vk_index_buffer, ctx->vk_index_buffer_memory, 0));
	}

	// create_staging_buffer
	{
		VkBufferCreateInfo buffer_info{ 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
			.size = sizeof(g_vertices) + sizeof(g_indices) + (ctx->surf->w * ctx->surf->h * 4),
			.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};
		VK_CHECK(vkCreateBuffer(ctx->vk_device, &buffer_info, nullptr, &ctx->vk_staging_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(ctx->vk_device, ctx->vk_staging_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info{ 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.size,
			.memoryTypeIndex = ctx->find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		};
		
		VK_CHECK(vkAllocateMemory(ctx->vk_device, &mem_info, nullptr, &ctx->vk_staging_buffer_memory));
		VK_CHECK(vkBindBufferMemory(ctx->vk_device, ctx->vk_staging_buffer, ctx->vk_staging_buffer_memory, 0));

		void* data;
		VK_CHECK(vkMapMemory(ctx->vk_device, ctx->vk_staging_buffer_memory, 0, buffer_info.size, 0, &data));
		SDL_memcpy(data, g_vertices.data(), sizeof(g_vertices));
		uint64_t offset = reinterpret_cast<uint64_t>(data) + sizeof(g_vertices);
		SDL_memcpy(reinterpret_cast<void*>(offset), g_indices.data(), sizeof(g_indices));
		offset += sizeof(g_indices);
		SDL_memcpy(reinterpret_cast<void*>(offset), ctx->surf->pixels, ctx->surf->w* ctx->surf->h * 4);
		vkUnmapMemory(ctx->vk_device, ctx->vk_staging_buffer_memory);
	}

	// create_uniform_buffer
	{
		VkBufferCreateInfo buffer_info{ 
			.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO, 
			.size = sizeof(UniformBufferObject) * 2,
			.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		};
		VK_CHECK(vkCreateBuffer(ctx->vk_device, &buffer_info, nullptr, &ctx->vk_uniform_buffer));

		VkMemoryRequirements mem_req;
		vkGetBufferMemoryRequirements(ctx->vk_device, ctx->vk_uniform_buffer, &mem_req);

		VkMemoryAllocateInfo mem_info{ 
			.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
			.allocationSize = mem_req.size,
			.memoryTypeIndex = ctx->find_memory_type(mem_req.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
		};

		VK_CHECK(vkAllocateMemory(ctx->vk_device, &mem_info, nullptr, &ctx->vk_uniform_buffer_memory));
		VK_CHECK(vkBindBufferMemory(ctx->vk_device, ctx->vk_uniform_buffer, ctx->vk_uniform_buffer_memory, 0));

		VK_CHECK(vkMapMemory(ctx->vk_device, ctx->vk_uniform_buffer_memory, 0, mem_req.size, 0, &ctx->vk_uniform_buffer_mapped));
	}

	// create_descriptor_pool
	{
		std::array<VkDescriptorPoolSize, 2> sizes{
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, MAX_FRAMES_IN_FLIGHT },
			VkDescriptorPoolSize{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_FRAMES_IN_FLIGHT },
		};

		VkDescriptorPoolCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
			.maxSets = MAX_FRAMES_IN_FLIGHT,
			.poolSizeCount = static_cast<uint32_t>(sizes.size()),
			.pPoolSizes = sizes.data(),
		};

		VK_CHECK(vkCreateDescriptorPool(ctx->vk_device, &info, nullptr, &ctx->vk_descriptor_pool));
	}

	// create_descriptor_sets
	{
		std::array<VkDescriptorSetLayout, MAX_FRAMES_IN_FLIGHT> layouts{ ctx->vk_descriptor_set_layout, ctx->vk_descriptor_set_layout };

		VkDescriptorSetAllocateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = ctx->vk_descriptor_pool,
			.descriptorSetCount = MAX_FRAMES_IN_FLIGHT,
			.pSetLayouts = layouts.data(),
		};
		VK_CHECK(vkAllocateDescriptorSets(ctx->vk_device, &info, ctx->vk_descriptor_set.data()));
	}

	// update_descriptor_sets
	{
		std::array<VkDescriptorBufferInfo, 2> buffer_infos{};
		VkDescriptorImageInfo image_info{};
		std::array<VkWriteDescriptorSet, 4> write_infos{};

		buffer_infos[0].buffer = ctx->vk_uniform_buffer;
		buffer_infos[0].offset = 0;
		buffer_infos[0].range = sizeof(UniformBufferObject);

		buffer_infos[1].buffer = ctx->vk_uniform_buffer;
		buffer_infos[1].offset = sizeof(UniformBufferObject);
		buffer_infos[1].range = sizeof(UniformBufferObject);

		image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		image_info.imageView = ctx->vk_image_view;
		image_info.sampler = ctx->vk_sampler;

		write_infos[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[0].dstSet = ctx->vk_descriptor_set[0];
		write_infos[0].dstBinding = 0;
		write_infos[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_infos[0].descriptorCount = 1;
		write_infos[0].pBufferInfo = &buffer_infos[0];

		write_infos[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[1].dstSet = ctx->vk_descriptor_set[1];
		write_infos[1].dstBinding = 0;
		write_infos[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		write_infos[1].descriptorCount = 1;
		write_infos[1].pBufferInfo = &buffer_infos[1];

		write_infos[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[2].dstSet = ctx->vk_descriptor_set[0];
		write_infos[2].dstBinding = 1;
		write_infos[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_infos[2].descriptorCount = 1;
		write_infos[2].pImageInfo = &image_info;

		write_infos[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write_infos[3].dstSet = ctx->vk_descriptor_set[1];
		write_infos[3].dstBinding = 1;
		write_infos[3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		write_infos[3].descriptorCount = 1;
		write_infos[3].pImageInfo = &image_info;
		
		vkUpdateDescriptorSets(ctx->vk_device, static_cast<uint32_t>(write_infos.size()), write_infos.data(), 0, nullptr);
	}

	// allocate_command_buffers
	{
		VkCommandBufferAllocateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = ctx->vk_command_pool,
			.commandBufferCount = static_cast<uint32_t>(ctx->vk_command_buffer.size()),
		};

		VK_CHECK(vkAllocateCommandBuffers(ctx->vk_device, &info, ctx->vk_command_buffer.data()));
	}

	// create_semaphores
	{
		VkSemaphoreCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
		};

		VK_CHECK(vkCreateSemaphore(ctx->vk_device, &info, nullptr, &ctx->vk_sem_image_available[0]));
		VK_CHECK(vkCreateSemaphore(ctx->vk_device, &info, nullptr, &ctx->vk_sem_render_finished[0]));
		VK_CHECK(vkCreateSemaphore(ctx->vk_device, &info, nullptr, &ctx->vk_sem_image_available[1]));
		VK_CHECK(vkCreateSemaphore(ctx->vk_device, &info, nullptr, &ctx->vk_sem_render_finished[1]));
	}

	// create_fences
	{
		VkFenceCreateInfo info{ 
			.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
			.flags = VK_FENCE_CREATE_SIGNALED_BIT,
		};

		VK_CHECK(vkCreateFence(ctx->vk_device, &info, nullptr, &ctx->vk_fence_in_flight[0]));
		VK_CHECK(vkCreateFence(ctx->vk_device, &info, nullptr, &ctx->vk_fence_in_flight[1]));
	}

	ctx->vk_graphics_queue = ctx->vk_queues[0][0]; // TODO

	ctx->arena_temp.reset();

	return SDL_APP_CONTINUE; 
}

SDL_AppResult SDL_AppIterate(void* appstate) {
	Context* ctx = static_cast<Context*>(appstate);

	VK_CHECK(vkWaitForFences(ctx->vk_device, 1, &ctx->vk_fence_in_flight[ctx->vk_current_frame], VK_TRUE, UINT64_MAX));
	VK_CHECK(vkResetFences(ctx->vk_device, 1, &ctx->vk_fence_in_flight[ctx->vk_current_frame]));

	uint32_t image_idx;
	VK_CHECK(vkAcquireNextImageKHR(ctx->vk_device, ctx->vk_swapchain, UINT64_MAX, ctx->vk_sem_image_available[ctx->vk_current_frame], VK_NULL_HANDLE, &image_idx));

	if (ctx->vk_current_frame == 0) {
		if (ctx->window_visible == WindowVisible::HIDE) {
			ctx->window_visible = WindowVisible::SHOW;
		}
		else if (ctx->window_visible == WindowVisible::SHOW) {
			SDL_CHECK(SDL_ShowWindow(ctx->window));
			ctx->window_visible = WindowVisible::SHOWN;
		}
	}

	VkCommandBuffer cb = ctx->vk_command_buffer[ctx->vk_current_frame];

	{
		VkCommandBufferBeginInfo info{ 
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
			.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		};
		VK_CHECK(vkBeginCommandBuffer(cb, &info));
	}

	if (!ctx->vk_staged) {
		ctx->vk_staged = true;

		{
			VkBufferCopy region{ 0, 0, sizeof(g_vertices) };
			vkCmdCopyBuffer(cb, ctx->vk_staging_buffer, ctx->vk_vertex_buffer, 1, &region);
		}
		{
			VkBufferCopy region{ sizeof(g_vertices), 0, sizeof(g_indices) };
			vkCmdCopyBuffer(cb, ctx->vk_staging_buffer, ctx->vk_index_buffer, 1, &region);
		}

		{
			VkImageMemoryBarrier barrier{ 
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = ctx->vk_image,
				.subresourceRange = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1 },
			};

			vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
		{
			VkBufferImageCopy region{
				.bufferOffset = sizeof(g_vertices) + sizeof(g_indices),
				.imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
				.imageExtent = ctx->vk_image_info.extent,
			};

			vkCmdCopyBufferToImage(cb, ctx->vk_staging_buffer, ctx->vk_image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
		}
		{
			VkImageMemoryBarrier barrier{ 
				.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
				.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
				.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
				.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
				.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
				.image = ctx->vk_image,
				.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
			};

			vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		}
	}
	else {
		VkImageMemoryBarrier barrier{ 
			.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
			.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
			.image = ctx->vk_image,
			.subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .levelCount = 1, .layerCount = 1},
		};
		vkCmdPipelineBarrier(cb, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
	}

	{
		std::array<VkClearValue, 2> clear_values{ VkClearValue
			{ .color = { .float32 = {0.0f, 0.0f, 0.0f, 1.0f}}},
			{ .depthStencil = { 1, 0 } },
		};

		VkRenderPassBeginInfo info{ 
			.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
			.renderPass = ctx->vk_render_pass,
			.framebuffer = ctx->vk_framebuffers[image_idx],
			.renderArea = { .extent = ctx->vk_swapchain_info.imageExtent },
			.clearValueCount = static_cast<uint32_t>(clear_values.size()),
			.pClearValues = clear_values.data(),

		};
		vkCmdBeginRenderPass(cb, &info, VK_SUBPASS_CONTENTS_INLINE);
	}

	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk_graphics_pipeline);
	vkCmdSetViewport(cb, 0, 1, &ctx->vk_viewport);
	vkCmdSetScissor(cb, 0, 1, &ctx->vk_scissor);

	{
		VkDeviceSize offset = 0;
		vkCmdBindVertexBuffers(cb, 0, 1, &ctx->vk_vertex_buffer, &offset);
	}

	vkCmdBindIndexBuffer(cb, ctx->vk_index_buffer, 0, VK_INDEX_TYPE_UINT16);

	vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, ctx->vk_pipeline_layout, 0, 1, &ctx->vk_descriptor_set[ctx->vk_current_frame], 0, nullptr);
	vkCmdDrawIndexed(cb, static_cast<uint32_t>(g_indices.size()), 1, 0, 0, 0);

	vkCmdEndRenderPass(cb); 

	// update_uniform_buffer
	{
		int64_t current_time;
		SDL_CHECK(SDL_GetCurrentTime(&current_time));
		const float NS_PER_SECOND = 1.0e9f;
		float time = static_cast<float>(current_time - ctx->start_time) / NS_PER_SECOND;

		UniformBufferObject ubo{};
		ubo.model = glm::rotate(glm::mat4{ 1.0f }, time * glm::radians(90.0f), glm::vec3{ 0.0f, 0.0f, 1.0f });
		ubo.view = glm::lookAt(glm::vec3{ 2.0f, 2.0f, 2.0f }, glm::vec3{ 0.0f, 0.0f, 0.0f }, glm::vec3{ 0.0f, 0.0f, 1.0f });
		ubo.proj = glm::perspective(glm::radians(45.0f), 16.0f / 9.0f, 0.1f, 10.0f);
		uint64_t dest = reinterpret_cast<uint64_t>(ctx->vk_uniform_buffer_mapped) + ctx->vk_current_frame * sizeof(UniformBufferObject);
		SDL_memcpy(reinterpret_cast<void*>(dest), &ubo, sizeof(ubo));
	}

	// submit_next_frame 
	{
		VK_CHECK(vkEndCommandBuffer(cb));

		VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

		VkSubmitInfo submit_info{ 
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &ctx->vk_sem_image_available[ctx->vk_current_frame],
			.pWaitDstStageMask = &wait_stage,
			.commandBufferCount = 1,
			.pCommandBuffers = &cb,
			.signalSemaphoreCount = 1,
			.pSignalSemaphores = &ctx->vk_sem_render_finished[ctx->vk_current_frame],
		};
		VK_CHECK(vkQueueSubmit(ctx->vk_graphics_queue, 1, &submit_info, ctx->vk_fence_in_flight[ctx->vk_current_frame]));

		VkPresentInfoKHR present_info{ 
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &ctx->vk_sem_render_finished[ctx->vk_current_frame],
			.swapchainCount = 1,
			.pSwapchains = &ctx->vk_swapchain,
			.pImageIndices = &image_idx,
		};
		VK_CHECK(vkQueuePresentKHR(ctx->vk_graphics_queue, &present_info));
	}

	ctx->vk_current_frame = (ctx->vk_current_frame + 1) % MAX_FRAMES_IN_FLIGHT;

	ctx->arena_temp.reset();

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
	SDL_Quit();
}


uint32_t Context::find_memory_type(uint32_t memory_types, VkMemoryPropertyFlags properties) {
	for (uint32_t memory_type = 0; memory_type < this->vk_physical_device_memory_properties.memoryTypeCount; memory_type++) {
		if (HAS_FLAG(memory_types, 1 << memory_type) && HAS_FLAG(this->vk_physical_device_memory_properties.memoryTypes[memory_type].propertyFlags, properties)) {
			return memory_type;
		}
	}
	return 0;
}

#pragma warning(pop)

/*
 
Input assembler: pass raw vertex data.
Vertex shader: runs for every vertex.
Tessellation shader: increases mesh quality.
Geometry shader: not used much anymore.
Rasterization stage: Turns primitives (triangles, lines, points) into fragments, AKA pixels.
Fragment shader: Runs for every fragment.
Color blending stage: Blends overlapping fragments into final pixels on the screen.

*/