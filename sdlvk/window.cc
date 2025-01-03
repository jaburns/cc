#include "inc.hh"
namespace {

VKAPI_ATTR VkBool32 VKAPI_CALL gfx_vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data
) {
    println("Vulkan validation layer: ", callback_data->pMessage);
    return VK_FALSE;
}

void GfxWindow::init(Arena* arena, cchar* window_title, SDL_AudioCallback sdl_audio_callback) {
    ZeroStruct(this);
    auto scratch = Arena::create(memory_get_global_allocator(), 0);

    auto validation_layers = Array(
        (cchar*)"VK_LAYER_KHRONOS_validation"
    );
    auto instance_extensions = Array(
        (cchar*)"VK_KHR_portability_enumeration",
        "VK_MVK_macos_surface",
        "VK_EXT_metal_surface",
        "VK_KHR_surface",
        "VK_EXT_debug_utils"
    );
    auto device_extensions = Array(
        (cchar*)"VK_KHR_portability_subset",
        "VK_KHR_swapchain"
    );
    f32 queue_priority = 1.f;

    // create window
    {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_AUDIO) < 0) {
            Panic("Failed to initialize SDL: %s\n", SDL_GetError());
        }

        sdl_window = SDL_CreateWindow(
            window_title,
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            1280, 720,
            SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN
        );

        if (!sdl_window) Panic("Failed to create SDL window: %s\n", SDL_GetError());
    }
    // create instance
    {
        auto available_validation_layers = VKGetSlice0(vkEnumerateInstanceLayerProperties, VkLayerProperties, &scratch);
        Assert(available_validation_layers.contains_all<cchar*>(
            validation_layers.slice(),
            [](auto a, auto b) { return cstr_eq(a->layerName, *b); }
        ));

        auto available_extensions = VKGetSlice(vkEnumerateInstanceExtensionProperties, VkExtensionProperties, &scratch, nullptr);
        Assert(available_extensions.contains_all<cchar*>(
            instance_extensions.slice(),
            [](auto a, auto b) { return cstr_eq(a->extensionName, *b); }
        ));

        auto app_info = VkApplicationInfo{
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = window_title,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "thirtythree",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_1,
        };
        auto create_info = VkInstanceCreateInfo{
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo        = &app_info,
            .enabledExtensionCount   = instance_extensions.count,
            .ppEnabledExtensionNames = instance_extensions.elems,
            .flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        };

        auto debug_create_info = VkDebugUtilsMessengerCreateInfoEXT{};

        if (ENABLE_VALIDATION_LAYERS) {
            create_info.enabledLayerCount   = 1;
            create_info.ppEnabledLayerNames = validation_layers.elems;

            debug_create_info = {
                .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = gfx_vulkan_debug_callback,
            };
            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
        } else {
            create_info.enabledLayerCount = 0;
            create_info.pNext             = nullptr;
        }

        VKExpect(vkCreateInstance(&create_info, nullptr, &instance), "Failed to create Vulkan instance");
        println("Vulkan instance created successfully");

        if (!SDL_Vulkan_CreateSurface(sdl_window, instance, &surface)) Panic("Failed to create Vulkan surface: %s\n", SDL_GetError());
    }
    // create device
    auto surface_capabilities = VkSurfaceCapabilitiesKHR{};
    auto surface_formats      = Slice<VkSurfaceFormatKHR>{};
    i32  graphics_queue_idx   = -1;
    {
        auto physical_devices = VKGetSlice(vkEnumeratePhysicalDevices, VkPhysicalDevice, &scratch, instance);
        if (physical_devices.count == 0) Panic("Failed to find GPUs with Vulkan support");

        i32 present_queue_idx;
        for (auto& pdevice : physical_devices) {
            physical_device    = pdevice;
            graphics_queue_idx = -1;
            present_queue_idx  = -1;

            auto available_extensions = VKGetSlice(vkEnumerateDeviceExtensionProperties, VkExtensionProperties, &scratch, physical_device, nullptr);

            bool all_supported = available_extensions.contains_all<cchar*>(
                device_extensions.slice(),
                [](auto a, auto b) { return cstr_eq(a->extensionName, *b); }
            );
            if (!all_supported) continue;

            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

            surface_formats = VKGetSlice(vkGetPhysicalDeviceSurfaceFormatsKHR, VkSurfaceFormatKHR, &scratch, physical_device, surface);
            if (surface_formats.count == 0) continue;
            auto present_modes = VKGetSlice(vkGetPhysicalDeviceSurfacePresentModesKHR, VkPresentModeKHR, &scratch, physical_device, surface);
            if (present_modes.count == 0) continue;

            auto queue_families = VKGetSlice(vkGetPhysicalDeviceQueueFamilyProperties, VkQueueFamilyProperties, &scratch, physical_device);

            for (i32 i = 0; i < queue_families.count; ++i) {
                if (queue_families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                    graphics_queue_idx = i;
                }
                VkBool32 present_support = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, surface, &present_support);
                if (present_support) {
                    present_queue_idx = i;
                }
            }

            if (graphics_queue_idx >= 0 && present_queue_idx >= 0) {
                break;
            }
        }
        if (graphics_queue_idx < 0) Panic("Failed to find a suitable GPU");

        auto present_queue_create_info = VkDeviceQueueCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = (u32)present_queue_idx,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
        auto gfx_queue_create_info = VkDeviceQueueCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = (u32)graphics_queue_idx,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
        auto device_features = VkPhysicalDeviceFeatures{};

        VkDeviceQueueCreateInfo infoz[] = {
            gfx_queue_create_info,
            present_queue_create_info,
        };

        auto create_info = VkDeviceCreateInfo{
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos       = infoz,
            .queueCreateInfoCount    = present_queue_idx == graphics_queue_idx ? 1u : 2u,
            .pEnabledFeatures        = &device_features,
            .enabledExtensionCount   = device_extensions.count,
            .ppEnabledExtensionNames = device_extensions.elems,
        };

        if (ENABLE_VALIDATION_LAYERS) {
            create_info.enabledLayerCount   = 1;
            create_info.ppEnabledLayerNames = validation_layers.elems;
        } else {
            create_info.enabledLayerCount = 0;
        }

        VKExpect(vkCreateDevice(physical_device, &create_info, nullptr, &device), "Failed to create logical device");

        vkGetDeviceQueue(device, graphics_queue_idx, 0, &graphics_queue);
        vkGetDeviceQueue(device, present_queue_idx, 0, &present_queue);
    }
    // create swap chain
    {
        VkExtent2D extent = surface_capabilities.currentExtent;
        if (surface_capabilities.currentExtent.width == UINT32_MAX) {
            SDL_Vulkan_GetDrawableSize(sdl_window, (i32*)&extent.width, (i32*)&extent.height);
            extent.width  = clamp(extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
            extent.height = clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
        }

        u32 format_idx = 0;
        for (u32 i = 0; i < surface_formats.count; ++i) {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_SRGB && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                format_idx = i;
                break;
            }
        }

        u32 image_count = surface_capabilities.minImageCount + 1;
        if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
            image_count = surface_capabilities.maxImageCount;
        }

        auto create_info = VkSwapchainCreateInfoKHR{
            .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .surface          = surface,
            .minImageCount    = image_count,
            .imageFormat      = surface_formats[format_idx].format,
            .imageColorSpace  = surface_formats[format_idx].colorSpace,
            .imageExtent      = extent,
            .imageArrayLayers = 1,
            .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
            .preTransform     = surface_capabilities.currentTransform,
            .compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
            .presentMode      = VK_PRESENT_MODE_FIFO_KHR,
            .clipped          = VK_TRUE,
            .oldSwapchain     = nullptr,
        };
        VKExpect(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain), "Failed to create swap chain");

        swap_chain_extent       = extent;
        swap_chain_image_format = surface_formats[format_idx].format;
        swap_chain_images       = VKGetSlice(vkGetSwapchainImagesKHR, VkImage, arena, device, swap_chain);
        swap_chain_image_views  = arena->alloc_many<VkImageView>(swap_chain_images.count);

        for (u32 i = 0; i < swap_chain_images.count; i++) {
            auto create_info = VkImageViewCreateInfo{
                .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image                           = swap_chain_images[i],
                .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
                .format                          = swap_chain_image_format,
                .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .subresourceRange.baseMipLevel   = 0,
                .subresourceRange.levelCount     = 1,
                .subresourceRange.baseArrayLayer = 0,
                .subresourceRange.layerCount     = 1,
            };
            VKExpect(vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views.elems[i]), "Failed to create image views");
        }
    }
    // create render pass
    {
        auto color_attachment = VkAttachmentDescription{
            .format         = swap_chain_image_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        auto color_attachment_ref = VkAttachmentReference{
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        auto subpass = VkSubpassDescription{
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &color_attachment_ref,
        };
        auto dependency = VkSubpassDependency{
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = 0,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        };
        auto render_pass_info = VkRenderPassCreateInfo{
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &color_attachment,
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = 1,
            .pDependencies   = &dependency,
        };
        VKExpect(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass), "Failed to create render pass");
    }
    // create graphics pipeline
    {
        Slice<u8> vert_code   = fs_read_file_bytes(&scratch, "shaders/bin/triangle.vertex.spv");
        auto      vert_module = VkShaderModule{};

        auto vert_create_info = VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vert_code.count,
            .pCode    = (u32*)vert_code.elems,
        };
        VKExpect(vkCreateShaderModule(device, &vert_create_info, nullptr, &vert_module), "Failed to create vert shader module");

        Slice<u8> frag_code   = fs_read_file_bytes(&scratch, "shaders/bin/triangle.fragment.spv");
        auto      frag_module = VkShaderModule{};

        auto frag_create_info = VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = frag_code.count,
            .pCode    = (u32*)frag_code.elems,
        };
        VKExpect(vkCreateShaderModule(device, &frag_create_info, nullptr, &frag_module), "Failed to create frag shader module");

        auto vert_shader_stage_info = VkPipelineShaderStageCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName  = "main",
        };
        auto frag_shader_stage_info = VkPipelineShaderStageCreateInfo{
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage  = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName  = "main",
        };
        VkPipelineShaderStageCreateInfo shader_stages[] = {
            vert_shader_stage_info,
            frag_shader_stage_info,
        };
        auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 0,
            .pVertexBindingDescriptions      = nullptr,
            .vertexAttributeDescriptionCount = 0,
            .pVertexAttributeDescriptions    = nullptr,
        };
        auto input_assembly = VkPipelineInputAssemblyStateCreateInfo{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        };
        VkDynamicState dynamic_states[] = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        auto dynamic_state = VkPipelineDynamicStateCreateInfo{
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = RawArrayLen(dynamic_states),
            .pDynamicStates    = dynamic_states,
        };
        auto viewport_state = VkPipelineViewportStateCreateInfo{
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };
        auto rasterizer = VkPipelineRasterizationStateCreateInfo{
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .lineWidth               = 1.f,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
        };
        auto multisampling = VkPipelineMultisampleStateCreateInfo{
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable  = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };
        auto color_blend_attachment = VkPipelineColorBlendAttachmentState{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable    = VK_FALSE,
        };
        auto color_blending = VkPipelineColorBlendStateCreateInfo{
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &color_blend_attachment,
        };

        auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };
        VKExpect(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout), "Failed to create pipeline layout");

        auto pipeline_info = VkGraphicsPipelineCreateInfo{
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = 2,
            .pStages             = shader_stages,
            .pVertexInputState   = &vertex_input_info,
            .pInputAssemblyState = &input_assembly,
            .pDynamicState       = &dynamic_state,
            .pViewportState      = &viewport_state,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = nullptr,
            .pColorBlendState    = &color_blending,
            .layout              = pipeline_layout,
            .renderPass          = render_pass,
            .subpass             = 0,
        };
        VKExpect(vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &graphics_pipeline), "Failed to create graphics pipeline");

        vkDestroyShaderModule(device, frag_module, nullptr);
        vkDestroyShaderModule(device, vert_module, nullptr);
    }
    // create framebuffers and command buffer
    {
        swap_chain_framebuffers = arena->alloc_many<VkFramebuffer>(swap_chain_image_views.count);

        for (u32 i = 0; i < swap_chain_image_views.count; i++) {
            VkImageView attachments[] = {
                swap_chain_image_views.elems[i]
            };

            auto framebuffer_info = VkFramebufferCreateInfo{
                .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
                .renderPass      = render_pass,
                .attachmentCount = 1,
                .pAttachments    = attachments,
                .width           = swap_chain_extent.width,
                .height          = swap_chain_extent.height,
                .layers          = 1,
            };
            VKExpect(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]), "Failed to create framebuffer");
        }

        auto pool_info = VkCommandPoolCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = (u32)graphics_queue_idx,
        };
        VKExpect(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool), "Failed to create command pool");

        auto alloc_info = VkCommandBufferAllocateInfo{
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };
        VKExpect(vkAllocateCommandBuffers(device, &alloc_info, &command_buffer), "Failed to allocate command buffers");
    }
    // create sync objects
    {
        auto semaphore_info = VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        auto fence_info = VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        VKExpect(vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphore), "Failed to create semaphore");
        VKExpect(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphore), "Failed to create semaphore");
        VKExpect(vkCreateFence(device, &fence_info, nullptr, &in_flight_fence), "Failed to create fence");
    }

    scratch.destroy();
}

bool GfxWindow::poll() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_QUIT: {
                return false;
            }
            case SDL_KEYDOWN: {
                return false;
            }
            default: {
            }
        }
    }
    return true;
}

void GfxWindow::swap() {
    vkWaitForFences(device, 1, &in_flight_fence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &in_flight_fence);

    u32 image_index;
    vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphore, nullptr, &image_index);
    vkResetCommandBuffer(command_buffer, 0);

    {
        auto begin_info = VkCommandBufferBeginInfo{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags            = 0,
            .pInheritanceInfo = nullptr,
        };
        VKExpect(vkBeginCommandBuffer(command_buffer, &begin_info), "Failed to begin recording command buffer");

        auto clear_color = VkClearValue{{{0.0f, 0.0f, 0.0f, 1.0f}}};

        auto render_pass_info = VkRenderPassBeginInfo{
            .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass        = render_pass,
            .framebuffer       = swap_chain_framebuffers[image_index],
            .renderArea.offset = {0, 0},
            .renderArea.extent = swap_chain_extent,
            .clearValueCount   = 1,
            .pClearValues      = &clear_color,
        };
        vkCmdBeginRenderPass(command_buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        auto viewport = VkViewport{
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = (f32)swap_chain_extent.width,
            .height   = (f32)swap_chain_extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(command_buffer, 0, 1, &viewport);

        auto scissor = VkRect2D{
            .offset = {0, 0},
            .extent = swap_chain_extent,
        };
        vkCmdSetScissor(command_buffer, 0, 1, &scissor);

        vkCmdDraw(command_buffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(command_buffer);

        VKExpect(vkEndCommandBuffer(command_buffer), "Failed to record command buffer");
    }

    VkSemaphore wait_semaphores[] = {
        image_available_semaphore
    };
    // wait until image available before running the color-attachment-output stage
    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkSemaphore signal_semaphores[] = {
        render_finished_semaphore,
    };
    VkSwapchainKHR swap_chains[] = {
        swap_chain,
    };

    auto submit_info = VkSubmitInfo{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = wait_semaphores,
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &command_buffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = signal_semaphores,
    };
    VKExpect(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence), "Failed to submit draw command buffer");

    auto present_info = VkPresentInfoKHR{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = signal_semaphores,
        .swapchainCount     = 1,
        .pSwapchains        = swap_chains,
        .pImageIndices      = &image_index,
    };

    vkQueuePresentKHR(present_queue, &present_info);
}

}  // namespace