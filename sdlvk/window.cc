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

struct Vertex {
    vec2 pos;
    vec3 color;

    static VkVertexInputBindingDescription get_binding_desc() {
        return VkVertexInputBindingDescription{
            .binding   = 0,
            .stride    = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    static Array<VkVertexInputAttributeDescription, 2> get_attribute_descs() {
        return Array<VkVertexInputAttributeDescription, 2>{{
            {
                .binding  = 0,
                .location = 0,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(Vertex, pos),
            },
            {
                .binding  = 0,
                .location = 1,
                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                .offset   = offsetof(Vertex, color),
            },
        }};
    }
};

Array<Vertex, 4> vertices = {{
    {vec2(-0.5f, -0.5f), vec3(1.0f, 0.0f, 0.0f)},
    {vec2(0.5f, -0.5f), vec3(0.0f, 1.0f, 0.0f)},
    {vec2(0.5f, 0.5f), vec3(0.0f, 0.0f, 1.0f)},
    {vec2(-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f)},
}};

Array<u16, 6> indices = {{0, 1, 2, 2, 3, 0}};

template <typename T, auto Fn, typename... Args>
Slice<T> vk_get_slice(Arena* arena, Args... args) {
    u32 count = 0;
    Fn(args..., &count, nullptr);
    if (count == 0) return {};
    Slice<T> ret_slice = arena->alloc_many<T>(count);
    Fn(args..., &count, ret_slice.elems);
    return ret_slice;
}

template <auto Fn, typename VecType, typename... Args>
void vk_get_vec(VecType* vec, Args... args) {
    u32 count = 0;
    Fn(args..., &count, nullptr);
    if (count == 0) {
        vec->count = 0;
        return;
    }
    if (count > vec->capacity) {
        Panic("Results from vk_get_into_vec would extend beyond the capacity of the target vec");
    }
    Fn(args..., &count, vec->elems);
    vec->count = count;
}

u32 vk_find_memory_type(VkPhysicalDevice physical_device, u32 type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);

    for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    Panic("vk_find_memory_type failed to find anything");
}

void vk_create_buffer(
    VkDevice device, VkPhysicalDevice physical_device, VkDeviceSize size,
    VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkBuffer* buffer, VkDeviceMemory* buffer_memory
) {
    auto vertex_buffer_info = VkBufferCreateInfo{
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,  // sizeof(Vertex) * vertices.count,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VKExpect(vkCreateBuffer(device, &vertex_buffer_info, nullptr, buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, *buffer, &mem_requirements);

    auto vertex_alloc_info = VkMemoryAllocateInfo{
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = vk_find_memory_type(physical_device, mem_requirements.memoryTypeBits, properties),
    };
    VKExpect(vkAllocateMemory(device, &vertex_alloc_info, nullptr, buffer_memory));

    vkBindBufferMemory(device, *buffer, *buffer_memory, 0);
}

void vk_copy_buffer(VkDevice device, VkQueue queue, VkCommandPool command_pool, VkBuffer dest, VkBuffer src, VkDeviceSize size) {
    auto allocInfo = VkCommandBufferAllocateInfo{
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = command_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, src, dest, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, command_pool, 1, &commandBuffer);
}

void GfxWindow::create_swap_chain() {
    log("creating swap chain");

    if (swap_chain) {
        vkDeviceWaitIdle(device);
        for (usize i = 0; i < swap_chain_framebuffers.count; ++i) {
            vkDestroyFramebuffer(device, swap_chain_framebuffers[i], nullptr);
        }
        for (usize i = 0; i < swap_chain_image_views.count; ++i) {
            vkDestroyImageView(device, swap_chain_image_views[i], nullptr);
        }
        vkDestroySwapchainKHR(device, swap_chain, nullptr);
    }

    VkSurfaceCapabilitiesKHR surface_capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, surface, &surface_capabilities);

    VkExtent2D extent = surface_capabilities.currentExtent;
    if (surface_capabilities.currentExtent.width == UINT32_MAX) {
        SDL_Vulkan_GetDrawableSize(sdl_window, (i32*)&extent.width, (i32*)&extent.height);
        extent.width  = clamp(extent.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
        extent.height = clamp(extent.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);
    }

    u32 image_count = surface_capabilities.minImageCount + 1;
    if (surface_capabilities.maxImageCount > 0 && image_count > surface_capabilities.maxImageCount) {
        image_count = surface_capabilities.maxImageCount;
    }

    auto create_info = VkSwapchainCreateInfoKHR{
        .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface          = surface,
        .minImageCount    = image_count,
        .imageFormat      = surface_format.format,
        .imageColorSpace  = surface_format.colorSpace,
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
    VKExpect(vkCreateSwapchainKHR(device, &create_info, nullptr, &swap_chain));

    swap_chain_extent = extent;
    vk_get_vec<vkGetSwapchainImagesKHR>(&swap_chain_images, device, swap_chain);

    swap_chain_image_views.count = swap_chain_images.count;
    for (u32 i = 0; i < swap_chain_image_views.count; ++i) {
        auto create_info = VkImageViewCreateInfo{
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image                           = swap_chain_images[i],
            .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
            .format                          = surface_format.format,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseMipLevel   = 0,
            .subresourceRange.levelCount     = 1,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
        };
        VKExpect(vkCreateImageView(device, &create_info, nullptr, &swap_chain_image_views[i]));
    }

    swap_chain_framebuffers.count = swap_chain_images.count;
    for (u32 i = 0; i < swap_chain_framebuffers.count; ++i) {
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
        VKExpect(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &swap_chain_framebuffers[i]));
    }
}

void GfxWindow::init(cchar* window_title, SDL_AudioCallback sdl_audio_callback) {
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
            SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE
        );

        if (!sdl_window) Panic("Failed to create SDL window: %s\n", SDL_GetError());

        SDL_AudioSpec obtained_spec;
        SDL_AudioSpec desired_spec = (SDL_AudioSpec){
            .freq     = AUDIO_SAMPLE_RATE,
            .format   = AUDIO_F32,
            .channels = 2,
            .samples  = 512,
            .callback = sdl_audio_callback,
            .userdata = this,
        };

        sdl_audio_device = SDL_OpenAudioDevice(NULL, 0, &desired_spec, &obtained_spec, 0);
        if (sdl_audio_device == 0) {
            Panic("Failed to open audio device: %s\n", SDL_GetError());
        }
        SDL_PauseAudioDevice(sdl_audio_device, 0);

        for (u32 i = 0; i < SDL_NumJoysticks(); ++i) {
            printf("Using joystick: %s\n", SDL_JoystickNameForIndex(i));
            sdl_joystick       = SDL_JoystickOpen(i);
            active_joystick_id = i;
            break;
        }
    }
    // create instance
    {
        auto available_validation_layers = vk_get_slice<VkLayerProperties, vkEnumerateInstanceLayerProperties>(&scratch);
        Assert(available_validation_layers.contains_all<cchar*>(
            validation_layers.slice(),
            [](auto a, auto b) { return cstr_eq(a->layerName, *b); }
        ));

        auto available_extensions = vk_get_slice<VkExtensionProperties, vkEnumerateInstanceExtensionProperties>(&scratch, nullptr);
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

        VKExpect(vkCreateInstance(&create_info, nullptr, &instance));
        log("vulkan instance created successfully");

        if (!SDL_Vulkan_CreateSurface(sdl_window, instance, &surface)) Panic("Failed to create Vulkan surface: %s\n", SDL_GetError());
    }
    // create device
    i32 graphics_queue_idx = -1;
    {
        auto physical_devices = vk_get_slice<VkPhysicalDevice, vkEnumeratePhysicalDevices>(&scratch, instance);
        if (physical_devices.count == 0) Panic("Failed to find GPUs with Vulkan support");

        Slice<VkSurfaceFormatKHR> surface_formats = {};

        i32 present_queue_idx;
        for (auto& pdevice : physical_devices) {
            physical_device    = pdevice;
            graphics_queue_idx = -1;
            present_queue_idx  = -1;

            auto available_extensions = vk_get_slice<VkExtensionProperties, vkEnumerateDeviceExtensionProperties>(&scratch, physical_device, nullptr);

            bool all_supported = available_extensions.contains_all<cchar*>(
                device_extensions.slice(),
                [](auto a, auto b) { return cstr_eq(a->extensionName, *b); }
            );
            if (!all_supported) continue;

            surface_formats = vk_get_slice<VkSurfaceFormatKHR, vkGetPhysicalDeviceSurfaceFormatsKHR>(&scratch, physical_device, surface);
            if (surface_formats.count == 0) continue;
            auto present_modes = vk_get_slice<VkPresentModeKHR, vkGetPhysicalDeviceSurfacePresentModesKHR>(&scratch, physical_device, surface);
            if (present_modes.count == 0) continue;

            auto queue_families = vk_get_slice<VkQueueFamilyProperties, vkGetPhysicalDeviceQueueFamilyProperties>(&scratch, physical_device);

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

        VKExpect(vkCreateDevice(physical_device, &create_info, nullptr, &device));

        vkGetDeviceQueue(device, graphics_queue_idx, 0, &graphics_queue);
        vkGetDeviceQueue(device, present_queue_idx, 0, &present_queue);

        for (u32 i = 0; i < surface_formats.count; ++i) {
            if (surface_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && surface_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                surface_format = surface_formats[i];
                break;
            }
        }
    }
    // create render pass
    {
        auto color_attachment = VkAttachmentDescription{
            .format         = surface_format.format,
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
        VKExpect(vkCreateRenderPass(device, &render_pass_info, nullptr, &render_pass));
    }

    create_swap_chain();

    // create graphics pipeline
    {
        Slice<u8> vert_code   = fs_read_file_bytes(&scratch, "shaders/bin/triangle.vertex.spv");
        auto      vert_module = VkShaderModule{};

        auto vert_create_info = VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vert_code.count,
            .pCode    = (u32*)vert_code.elems,
        };
        VKExpect(vkCreateShaderModule(device, &vert_create_info, nullptr, &vert_module));

        Slice<u8> frag_code   = fs_read_file_bytes(&scratch, "shaders/bin/triangle.fragment.spv");
        auto      frag_module = VkShaderModule{};

        auto frag_create_info = VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = frag_code.count,
            .pCode    = (u32*)frag_code.elems,
        };
        VKExpect(vkCreateShaderModule(device, &frag_create_info, nullptr, &frag_module));

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

        auto vertex_binding_desc    = Vertex::get_binding_desc();
        auto vertex_attribute_descs = Vertex::get_attribute_descs();

        auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &vertex_binding_desc,
            .vertexAttributeDescriptionCount = (u32)vertex_attribute_descs.count,
            .pVertexAttributeDescriptions    = vertex_attribute_descs.elems,
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
        VKExpect(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &pipeline_layout));

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
        VKExpect(vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &graphics_pipeline));

        vkDestroyShaderModule(device, frag_module, nullptr);
        vkDestroyShaderModule(device, vert_module, nullptr);

        auto pool_info = VkCommandPoolCreateInfo{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = (u32)graphics_queue_idx,
        };
        VKExpect(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));

        // vertex buffer
        {
            VkDeviceSize size = sizeof(Vertex) * vertices.count;

            VkBuffer       staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            vk_create_buffer(
                device, physical_device, size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &staging_buffer, &staging_buffer_memory
            );

            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, size, 0, &data);
            memcpy(data, vertices.elems, size);
            vkUnmapMemory(device, staging_buffer_memory);

            vk_create_buffer(
                device, physical_device, size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &vertex_buffer, &vertex_buffer_memory
            );

            vk_copy_buffer(device, graphics_queue, command_pool, vertex_buffer, staging_buffer, size);

            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_buffer_memory, nullptr);
        }
        // index buffer
        {
            VkDeviceSize size = sizeof(u16) * indices.count;

            VkBuffer       staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            vk_create_buffer(
                device, physical_device, size,
                VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                &staging_buffer, &staging_buffer_memory
            );

            void* data;
            vkMapMemory(device, staging_buffer_memory, 0, size, 0, &data);
            memcpy(data, indices.elems, size);
            vkUnmapMemory(device, staging_buffer_memory);

            vk_create_buffer(
                device, physical_device, size,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                &index_buffer, &index_buffer_memory
            );

            vk_copy_buffer(device, graphics_queue, command_pool, index_buffer, staging_buffer, size);

            vkDestroyBuffer(device, staging_buffer, nullptr);
            vkFreeMemory(device, staging_buffer_memory, nullptr);
        }

        auto alloc_info = VkCommandBufferAllocateInfo{
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 2,
        };
        VKExpect(vkAllocateCommandBuffers(device, &alloc_info, command_buffers.elems));

        auto semaphore_info = VkSemaphoreCreateInfo{
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        auto fence_info = VkFenceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
            VKExpect(vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphores[i]));
            VKExpect(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphores[i]));
            VKExpect(vkCreateFence(device, &fence_info, nullptr, &in_flight_fences[i]));
        }
    }
#if EDITOR
    {
        ImGui::CreateContext();
        ImGui_ImplSDL2_InitForVulkan(sdl_window);

        VkDescriptorPoolSize pool_sizes[] = {
            {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
            {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
            {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
            {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}
        };

        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets                    = 1000;
        pool_info.poolSizeCount              = RawArrayLen(pool_sizes);
        pool_info.pPoolSizes                 = pool_sizes;

        VkDescriptorPool imgui_pool;
        VKExpect(vkCreateDescriptorPool(device, &pool_info, nullptr, &imgui_pool));

        auto init_info = ImGui_ImplVulkan_InitInfo{
            .Instance       = instance,
            .PhysicalDevice = physical_device,
            .Device         = device,
            .Queue          = graphics_queue,
            .QueueFamily    = (u32)graphics_queue_idx,
            .DescriptorPool = imgui_pool,
            .MinImageCount  = 3,
            .ImageCount     = 3,
            .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
            .RenderPass     = render_pass,
        };

        ImGui_ImplVulkan_Init(&init_info);
    }
#endif

    scratch.destroy();
}

void GfxWindow::wait_device_idle() {
    vkDeviceWaitIdle(device);
}

bool GfxWindow::poll() {
    mouse_delta       = IVEC2_ZERO;
    mouse_delta_wheel = 0.f;

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
#if EDITOR
        ImGui_ImplSDL2_ProcessEvent(&event);
        bool imgui_wants_mouse    = ImGui::GetIO().WantCaptureMouse;
        bool imgui_wants_keyboard = ImGui::GetIO().WantCaptureKeyboard;
#else
        constexpr bool imgui_wants_mouse    = false;
        constexpr bool imgui_wants_keyboard = false;
#endif

        switch (event.type) {
            case SDL_WINDOWEVENT: {
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_RESIZED: {
                        framebuffer_resized = true;
                        // i32 w         = event.window.data1;
                        // i32 h         = event.window.data2;
                        // screen_size.x = w;
                        // screen_size.y = h;
                        break;
                    }
                }
                break;
            }
            case SDL_MOUSEBUTTONDOWN: {
                if (!imgui_wants_mouse) {
                    mouse_button = true;
                }
                break;
            }
            case SDL_MOUSEBUTTONUP: {
                if (!imgui_wants_mouse) {
                    mouse_button = false;
                }
                break;
            }
            case SDL_MOUSEMOTION: {
                if (!imgui_wants_mouse) {
                    mouse_delta.x = event.motion.xrel;
                    mouse_delta.y = -event.motion.yrel;
                }
                break;
            }
            case SDL_MOUSEWHEEL: {
                if (!imgui_wants_mouse) {
                    mouse_delta_wheel = event.wheel.preciseY;
                }
                break;
            }
            case SDL_QUIT: {
                return false;
            }
            case SDL_KEYDOWN: {
                if (!imgui_wants_keyboard) {
                    keyboard.scancodes_down[event.key.keysym.scancode] = true;
                }
                break;
            }
            case SDL_KEYUP: {
                if (!imgui_wants_keyboard) {
                    keyboard.scancodes_down[event.key.keysym.scancode] = false;
                }
                break;
            }
            case SDL_JOYAXISMOTION: {
                // printf("Joystick %d axis %d moved to %d\n", event.jaxis.which, event.jaxis.axis, event.jaxis.value);
                if (event.jaxis.which == active_joystick_id && event.jaxis.axis < JoystickState::AXIS_COUNT) {
                    joystick.axis_values[event.jaxis.axis] =
                        event.jaxis.value < 0 ? (f32)event.jaxis.value / 32768.0 : (f32)event.jaxis.value / 32767.0;
                }
                break;
            }
            case SDL_JOYBUTTONDOWN: {
                // printf("Joystick %d button %d pressed\n", event.jbutton.which, event.jbutton.button);
                if (event.jbutton.which == active_joystick_id && event.jbutton.button < JoystickState::BUTTON_COUNT) {
                    joystick.buttons_down[event.jbutton.button] = true;
                }
                break;
            }
            case SDL_JOYBUTTONUP: {
                // printf("Joystick %d button %d released\n", event.jbutton.which, event.jbutton.button);
                if (event.jbutton.which == active_joystick_id && event.jbutton.button < JoystickState::BUTTON_COUNT) {
                    joystick.buttons_down[event.jbutton.button] = false;
                }
                break;
            }
        }
    }

#if EDITOR
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
#endif

    return true;
}

void GfxWindow::swap() {
top:
    vkWaitForFences(device, 1, &in_flight_fences[cur_framebuffer_idx], VK_TRUE, UINT64_MAX);

    u32      image_index;
    VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphores[cur_framebuffer_idx], nullptr, &image_index);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        log("vkAcquireNextImageKHR result: ", result);
        framebuffer_resized = false;

        // dummy submission to reset the image_available semaphore
        {
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

            auto dummy_submit_info = VkSubmitInfo{
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = &image_available_semaphores[cur_framebuffer_idx],
                .pWaitDstStageMask    = waitStages,
                .commandBufferCount   = 0,
                .pCommandBuffers      = nullptr,
                .signalSemaphoreCount = 0,
                .pSignalSemaphores    = nullptr,
            };
            vkQueueSubmit(graphics_queue, 1, &dummy_submit_info, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphics_queue);
        }

        create_swap_chain();
        goto top;
    } else if (result != VK_SUCCESS) {
        Panic("Failed to acquire swap chain image");
    }

    vkResetFences(device, 1, &in_flight_fences[cur_framebuffer_idx]);

#if EDITOR
    ImGui::Render();
#endif

    {
        auto& buffer = command_buffers[cur_framebuffer_idx];
        vkResetCommandBuffer(buffer, 0);

        auto begin_info = VkCommandBufferBeginInfo{
            .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags            = 0,
            .pInheritanceInfo = nullptr,
        };
        VKExpect(vkBeginCommandBuffer(buffer, &begin_info));

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
        vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphics_pipeline);

        auto viewport = VkViewport{
            .x        = 0.0f,
            .y        = 0.0f,
            .width    = (f32)swap_chain_extent.width,
            .height   = (f32)swap_chain_extent.height,
            .minDepth = 0.0f,
            .maxDepth = 1.0f,
        };
        vkCmdSetViewport(buffer, 0, 1, &viewport);

        auto scissor = VkRect2D{
            .offset = {0, 0},
            .extent = swap_chain_extent,
        };
        vkCmdSetScissor(buffer, 0, 1, &scissor);

        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(buffer, 0, 1, &vertex_buffer, offsets);
        vkCmdBindIndexBuffer(buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(buffer, (u32)indices.count, 1, 0, 0, 0);

#if EDITOR
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);
#endif

        vkCmdEndRenderPass(buffer);

        VKExpect(vkEndCommandBuffer(buffer));
    }

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    auto submit_info = VkSubmitInfo{
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &image_available_semaphores[cur_framebuffer_idx],
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &command_buffers[cur_framebuffer_idx],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &render_finished_semaphores[cur_framebuffer_idx],
    };
    VKExpect(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences[cur_framebuffer_idx]));

    auto present_info = VkPresentInfoKHR{
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &render_finished_semaphores[cur_framebuffer_idx],
        .swapchainCount     = 1,
        .pSwapchains        = &swap_chain,
        .pImageIndices      = &image_index,
    };

    result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        log("vkQueuePresentKHR result: ", result);
        framebuffer_resized = false;
        create_swap_chain();
    } else if (result != VK_SUCCESS) {
        Panic("Failed to present swap chain image");
    }

    cur_framebuffer_idx = (cur_framebuffer_idx + 1) % MAX_FRAMES_IN_FLIGHT;
}

}  // namespace