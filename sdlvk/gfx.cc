#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

VkSampleCountFlagBits vk_get_max_usable_sample_count(VkPhysicalDevice physical_device) {
    VkPhysicalDeviceProperties props;
    vkGetPhysicalDeviceProperties(physical_device, &props);

    VkSampleCountFlags counts = props.limits.framebufferColorSampleCounts & props.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) return VK_SAMPLE_COUNT_64_BIT;
    if (counts & VK_SAMPLE_COUNT_32_BIT) return VK_SAMPLE_COUNT_32_BIT;
    if (counts & VK_SAMPLE_COUNT_16_BIT) return VK_SAMPLE_COUNT_16_BIT;
    if (counts & VK_SAMPLE_COUNT_8_BIT) return VK_SAMPLE_COUNT_8_BIT;
    if (counts & VK_SAMPLE_COUNT_4_BIT) return VK_SAMPLE_COUNT_4_BIT;
    if (counts & VK_SAMPLE_COUNT_2_BIT) return VK_SAMPLE_COUNT_2_BIT;
    return VK_SAMPLE_COUNT_1_BIT;
}

// -----------------------------------------------------------------------------

void Gfx::create_swap_chain() {
    vkDeviceWaitIdle(device);
    swap_chain_drop_pool.drop_all(device);

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

    VkSwapchainCreateInfoKHR create_info = {
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
    swap_chain_drop_pool.push(vkDestroySwapchainKHR, swap_chain);

    screen_size.x = extent.width;
    screen_size.y = extent.height;
    vk_get_vec<vkGetSwapchainImagesKHR>(&swap_chain_images, device, swap_chain);

    swap_chain_image_views.count = swap_chain_images.count;
    for (u32 i = 0; i < swap_chain_image_views.count; ++i) {
        VkImageViewCreateInfo create_info = {
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
        swap_chain_drop_pool.push(vkDestroyImageView, swap_chain_image_views[i]);
    }

    // depth

    vk_create_image(
        extent.width, extent.height, 1, VK_SAMPLE_COUNT_1_BIT,
        depth_format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &swap_chain_drop_pool, &depth_image, &depth_image_memory
    );
    VkImageViewCreateInfo depth_view_info = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                           = depth_image,
        .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
        .format                          = depth_format,
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_DEPTH_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = 1,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
    };
    VKExpect(vkCreateImageView(device, &depth_view_info, nullptr, &depth_image_view));
    swap_chain_drop_pool.push(vkDestroyImageView, depth_image_view);

    // framebuffers

    main_pass_framebuffers.count = swap_chain_image_views.count;
    for (u32 i = 0; i < main_pass_framebuffers.count; ++i) {
        VkImageView attachments[] = {
            swap_chain_image_views.elems[i],
            depth_image_view,
        };
        VkFramebufferCreateInfo framebuffer_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = main_pass,
            .attachmentCount = RawArrayLen(attachments),
            .pAttachments    = attachments,
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1,
        };
        VKExpect(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &main_pass_framebuffers[i]));
        swap_chain_drop_pool.push(vkDestroyFramebuffer, main_pass_framebuffers[i]);
    }

#if EDITOR
    imgui_framebuffers.count = swap_chain_image_views.count;
    for (u32 i = 0; i < imgui_framebuffers.count; ++i) {
        VkImageView attachments[] = {
            swap_chain_image_views.elems[i],
        };
        VkFramebufferCreateInfo framebuffer_info = {
            .sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass      = imgui_render_pass,
            .attachmentCount = 1,
            .pAttachments    = attachments,
            .width           = extent.width,
            .height          = extent.height,
            .layers          = 1,
        };
        VKExpect(vkCreateFramebuffer(device, &framebuffer_info, nullptr, &imgui_framebuffers[i]));
        swap_chain_drop_pool.push(vkDestroyFramebuffer, imgui_framebuffers[i]);
    }
#endif
}

VkFormat vk_find_supported_format(VkPhysicalDevice physical_device, Slice<VkFormat> candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physical_device, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    Panic("Failed to find supported format");
}
bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

// -----------------------------------------------------------------------------

VKAPI_ATTR VkBool32 VKAPI_CALL vk_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT      message_severity,
    VkDebugUtilsMessageTypeFlagsEXT             message_type,
    const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
    void*                                       user_data
) {
    println("\033[1;35mVulkan validation layer:\033[0m ", callback_data->pMessage);
    return VK_FALSE;
}

// -----------------------------------------------------------------------------

void Gfx::init(Arena* arena, cchar* window_title, SDL_AudioCallback sdl_audio_callback) {
    ZeroStruct(this);
    Arena scratch = Arena::create(memory_get_global_allocator());
    defer { scratch.destroy(); };

    cchar* validation_layers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    cchar* instance_extensions[] = {
        "VK_KHR_portability_enumeration",
        "VK_MVK_macos_surface",
        "VK_EXT_metal_surface",
        "VK_KHR_surface",
        "VK_EXT_debug_utils"
    };
    cchar* device_extensions[] = {
        "VK_KHR_portability_subset",
        "VK_KHR_swapchain"
    };
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
            SliceFromRawArray(cchar*, validation_layers),
            [](VkLayerProperties* a, cchar** b) { return cstr_eq(a->layerName, *b); }
        ));

        auto available_extensions = vk_get_slice<VkExtensionProperties, vkEnumerateInstanceExtensionProperties>(&scratch, nullptr);
        Assert(available_extensions.contains_all<cchar*>(
            SliceFromRawArray(cchar*, instance_extensions),
            [](VkExtensionProperties* a, cchar** b) { return cstr_eq(a->extensionName, *b); }
        ));

        VkApplicationInfo app_info = {
            .sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName   = window_title,
            .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
            .pEngineName        = "thirtythree",
            .engineVersion      = VK_MAKE_VERSION(1, 0, 0),
            .apiVersion         = VK_API_VERSION_1_2,
        };
        VkInstanceCreateInfo create_info = {
            .sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo        = &app_info,
            .enabledExtensionCount   = RawArrayLen(instance_extensions),
            .ppEnabledExtensionNames = instance_extensions,
            .flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
        };

        VkDebugUtilsMessengerCreateInfoEXT debug_create_info = {};

        if (ENABLE_VALIDATION_LAYERS) {
            create_info.enabledLayerCount   = RawArrayLen(validation_layers);
            create_info.ppEnabledLayerNames = validation_layers;

            debug_create_info = {
                .sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                .pfnUserCallback = vk_debug_callback,
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
        for (VkPhysicalDevice& pdevice : physical_devices) {
            physical_device    = pdevice;
            graphics_queue_idx = -1;
            present_queue_idx  = -1;

            auto available_extensions = vk_get_slice<VkExtensionProperties, vkEnumerateDeviceExtensionProperties>(&scratch, physical_device, nullptr);

            bool all_supported = available_extensions.contains_all<cchar*>(
                SliceFromRawArray(cchar*, device_extensions),
                [](VkExtensionProperties* a, cchar** b) { return cstr_eq(a->extensionName, *b); }
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

        VkDeviceQueueCreateInfo present_queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = (u32)present_queue_idx,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
        VkDeviceQueueCreateInfo gfx_queue_create_info = {
            .sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .queueFamilyIndex = (u32)graphics_queue_idx,
            .queueCount       = 1,
            .pQueuePriorities = &queue_priority,
        };
        VkPhysicalDeviceFeatures device_features = {
            .samplerAnisotropy = VK_TRUE,
        };

        VkDeviceQueueCreateInfo infoz[] = {
            gfx_queue_create_info,
            present_queue_create_info,
        };

        VkDeviceCreateInfo create_info = {
            .sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pQueueCreateInfos       = infoz,
            .queueCreateInfoCount    = present_queue_idx == graphics_queue_idx ? 1u : 2u,
            .pEnabledFeatures        = &device_features,
            .enabledExtensionCount   = RawArrayLen(device_extensions),
            .ppEnabledExtensionNames = device_extensions,
        };

        if (ENABLE_VALIDATION_LAYERS) {
            create_info.enabledLayerCount   = RawArrayLen(validation_layers);
            create_info.ppEnabledLayerNames = validation_layers;
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

    // msaa_samples = vk_get_max_usable_sample_count(physical_device);

    VkFormat formats[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    depth_format       = vk_find_supported_format(physical_device, SliceFromRawArray(VkFormat, formats), VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // create main render pass
    {
        VkAttachmentDescription color_attachment = {
            .format         = surface_format.format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        VkAttachmentReference color_attachment_ref = {
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        };
        VkAttachmentDescription depth_attachment = {
            .format         = depth_format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
            .finalLayout    = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };
        VkAttachmentReference depth_attachment_ref = {
            .attachment = 1,
            .layout     = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        };

        VkSubpassDescription subpass = {
            .pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount    = 1,
            .pColorAttachments       = &color_attachment_ref,
            .pDepthStencilAttachment = &depth_attachment_ref,
        };
        VkSubpassDependency dependency = {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .srcAccessMask = 0,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        };

        VkAttachmentDescription attachments[] = {
            color_attachment,
            depth_attachment,
        };
        VkRenderPassCreateInfo render_pass_info = {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = RawArrayLen(attachments),
            .pAttachments    = attachments,
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = 1,
            .pDependencies   = &dependency,
        };
        VKExpect(vkCreateRenderPass(device, &render_pass_info, nullptr, &main_pass));
    }
    // create command pool and presentation sync objects
    {
        VkCommandPoolCreateInfo pool_info = {
            .sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
            .flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
            .queueFamilyIndex = (u32)graphics_queue_idx,
        };
        VKExpect(vkCreateCommandPool(device, &pool_info, nullptr, &command_pool));

        VkCommandBufferAllocateInfo alloc_info = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = command_pool,
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 2,
        };
        VKExpect(vkAllocateCommandBuffers(device, &alloc_info, command_buffer.elems));

        VkSemaphoreCreateInfo semaphore_info = {
            .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        };
        VkFenceCreateInfo fence_info = {
            .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
            .flags = VK_FENCE_CREATE_SIGNALED_BIT,
        };
        for (u32 i = 0; i < GFX_MAX_FRAMES_IN_FLIGHT; ++i) {
            VKExpect(vkCreateSemaphore(device, &semaphore_info, nullptr, &image_available_semaphore[i]));
            VKExpect(vkCreateSemaphore(device, &semaphore_info, nullptr, &render_finished_semaphore[i]));
            VKExpect(vkCreateFence(device, &fence_info, nullptr, &in_flight_fence[i]));
        }
    }
#if EDITOR
    // create imgui renderpass and init imgui
    {
        VkAttachmentDescription color_attachment = {
            .format         = surface_format.format,
            .samples        = VK_SAMPLE_COUNT_1_BIT,
            .loadOp         = VK_ATTACHMENT_LOAD_OP_LOAD,
            .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
            .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
            .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
            .initialLayout  = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        };
        VkAttachmentReference color_attachment_ref = {
            .attachment = 0,
            .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
        };
        VkSubpassDescription subpass = {
            .pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS,
            .colorAttachmentCount = 1,
            .pColorAttachments    = &color_attachment_ref,
        };
        VkSubpassDependency dependency = {
            .srcSubpass    = VK_SUBPASS_EXTERNAL,
            .dstSubpass    = 0,
            .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        };
        VkRenderPassCreateInfo render_pass_info = {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
            .attachmentCount = 1,
            .pAttachments    = &color_attachment,
            .subpassCount    = 1,
            .pSubpasses      = &subpass,
            .dependencyCount = 1,
            .pDependencies   = &dependency,
        };
        VKExpect(vkCreateRenderPass(device, &render_pass_info, nullptr, &imgui_render_pass));

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

        ImGui_ImplVulkan_InitInfo init_info = {
            .Instance       = instance,
            .PhysicalDevice = physical_device,
            .Device         = device,
            .Queue          = graphics_queue,
            .QueueFamily    = (u32)graphics_queue_idx,
            .DescriptorPool = imgui_pool,
            .MinImageCount  = 3,
            .ImageCount     = 3,
            .MSAASamples    = VK_SAMPLE_COUNT_1_BIT,
            .RenderPass     = imgui_render_pass,
        };

        ImGui_ImplVulkan_Init(&init_info);
    }
#endif

    swap_chain_drop_pool = VKDropPool::alloc(arena, 32);
    create_swap_chain();
}

// -----------------------------------------------------------------------------

bool Gfx::poll() {
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

void Gfx::wait_queue_idle() {
    vkQueueWaitIdle(graphics_queue);
}

void Gfx::wait_device_idle() {
    vkDeviceWaitIdle(device);
}

// -----------------------------------------------------------------------------

VkCommandBuffer Gfx::main_render_pass_begin(vec4 color_clear, f32 depth_clear, u32 stencil_clear) {
top:
    vkWaitForFences(device, 1, &in_flight_fence[cur_framebuffer_idx], VK_TRUE, UINT64_MAX);

    VkResult result = vkAcquireNextImageKHR(device, swap_chain, UINT64_MAX, image_available_semaphore[cur_framebuffer_idx], nullptr, &cur_image_idx);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        log("vkAcquireNextImageKHR result: ", result);
        framebuffer_resized = false;

        // dummy submission to reset the image_available semaphore
        {
            VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT};

            VkSubmitInfo dummy_submit_info = {
                .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .waitSemaphoreCount   = 1,
                .pWaitSemaphores      = &image_available_semaphore[cur_framebuffer_idx],
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

    vkResetFences(device, 1, &in_flight_fence[cur_framebuffer_idx]);

    auto& buffer = command_buffer[cur_framebuffer_idx];
    vkResetCommandBuffer(buffer, 0);

    VkCommandBufferBeginInfo begin_info = {
        .sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags            = 0,
        .pInheritanceInfo = nullptr,
    };
    VKExpect(vkBeginCommandBuffer(buffer, &begin_info));

    VkExtent2D extent = {(u32)screen_size.x, (u32)screen_size.y};

    VkClearValue clear_values[] = {
        {.color = {{color_clear.r, color_clear.g, color_clear.b, color_clear.a}}},
        {.depthStencil = {depth_clear, stencil_clear}},
    };

    VkRenderPassBeginInfo render_pass_info = {
        .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass        = main_pass,
        .framebuffer       = main_pass_framebuffers[cur_image_idx],
        .renderArea.offset = {0, 0},
        .renderArea.extent = extent,
        .clearValueCount   = 2,
        .pClearValues      = clear_values,
    };
    vkCmdBeginRenderPass(buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    return buffer;
}

void Gfx::main_render_pass_end() {
    VkCommandBuffer& buffer = command_buffer[cur_framebuffer_idx];
    vkCmdEndRenderPass(buffer);

#if EDITOR
    ImGui::Render();

    VkExtent2D extent = {(u32)screen_size.x, (u32)screen_size.y};

    VkRenderPassBeginInfo imgui_rp_begin = {
        .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass      = imgui_render_pass,
        .framebuffer     = imgui_framebuffers[cur_image_idx],
        .renderArea      = {{0, 0}, extent},
        .clearValueCount = 0,
        .pClearValues    = nullptr,
    };
    vkCmdBeginRenderPass(buffer, &imgui_rp_begin, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), buffer);
    vkCmdEndRenderPass(buffer);
#endif
    VKExpect(vkEndCommandBuffer(buffer));

    VkPipelineStageFlags wait_stages[] = {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    VkSubmitInfo submit_info = {
        .sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount   = 1,
        .pWaitSemaphores      = &image_available_semaphore[cur_framebuffer_idx],
        .pWaitDstStageMask    = wait_stages,
        .commandBufferCount   = 1,
        .pCommandBuffers      = &command_buffer[cur_framebuffer_idx],
        .signalSemaphoreCount = 1,
        .pSignalSemaphores    = &render_finished_semaphore[cur_framebuffer_idx],
    };
    VKExpect(vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fence[cur_framebuffer_idx]));

    VkPresentInfoKHR present_info = {
        .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores    = &render_finished_semaphore[cur_framebuffer_idx],
        .swapchainCount     = 1,
        .pSwapchains        = &swap_chain,
        .pImageIndices      = &cur_image_idx,
    };

    VkResult result = vkQueuePresentKHR(present_queue, &present_info);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized) {
        log("vkQueuePresentKHR result: ", result);
        framebuffer_resized = false;
        create_swap_chain();
    } else if (result != VK_SUCCESS) {
        Panic("Failed to present swap chain image");
    }

    cur_framebuffer_idx = (cur_framebuffer_idx + 1) % GFX_MAX_FRAMES_IN_FLIGHT;
}

// -----------------------------------------------------------------------------

forall(T) VkBuffer Gfx::vk_create_device_local_buffer(VKDropPool* drop_pool, Slice<T> data, VkBufferUsageFlags usage_flags) {
    ScratchArena scratch{};
    VKDropPool   vk_scratch = VKDropPool::alloc(scratch.arena, 4);
    defer { vk_scratch.drop_all(device); };

    VkDeviceSize   size = sizeof(T) * data.count;
    VkBuffer       staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    vk_create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &vk_scratch, &staging_buffer, &staging_buffer_memory
    );

    void* mapped;
    vkMapMemory(device, staging_buffer_memory, 0, size, 0, &mapped);
    data.copy_into(mapped);
    vkUnmapMemory(device, staging_buffer_memory);

    VkDeviceMemory buffer_memory;
    VkBuffer       buffer;

    vk_create_buffer(
        size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage_flags, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        drop_pool, &buffer, &buffer_memory
    );
    vk_copy_buffer(buffer, staging_buffer, size);

    return buffer;
}

// -----------------------------------------------------------------------------

void Gfx::vk_create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VKDropPool* drop_pool, VkBuffer* out_buffer, VkDeviceMemory* out_buffer_memory) {
    VkBufferCreateInfo create_info = {
        .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size        = size,  // sizeof(Vertex) * vertices.count,
        .usage       = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };
    VKExpect(vkCreateBuffer(device, &create_info, nullptr, out_buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(device, *out_buffer, &mem_requirements);

    u32 mem_type_idx = -1;
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
        for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((mem_requirements.memoryTypeBits & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                mem_type_idx = i;
                break;
            }
        }
        if (mem_type_idx == -1) {
            Panic("Failed to find memory type");
        }
    }

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = mem_type_idx,
    };

    VKExpect(vkAllocateMemory(device, &alloc_info, nullptr, out_buffer_memory));

    vkBindBufferMemory(device, *out_buffer, *out_buffer_memory, 0);

    drop_pool->push(vkDestroyBuffer, *out_buffer);
    drop_pool->push(vkFreeMemory, *out_buffer_memory);
}

void Gfx::vk_create_image(u32 width, u32 height, u32 mip_levels, VkSampleCountFlagBits num_samples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VKDropPool* drop_pool, VkImage* out_image, VkDeviceMemory* out_image_memory) {
    VkImageCreateInfo image_info = {
        .sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType     = VK_IMAGE_TYPE_2D,
        .extent.width  = width,
        .extent.height = height,
        .extent.depth  = 1,
        .mipLevels     = mip_levels,
        .arrayLayers   = 1,
        .format        = format,
        .tiling        = tiling,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .usage         = usage,
        .samples       = num_samples,
        .sharingMode   = VK_SHARING_MODE_EXCLUSIVE,
    };
    VKExpect(vkCreateImage(device, &image_info, nullptr, out_image));

    VkMemoryRequirements mem_requirements;
    vkGetImageMemoryRequirements(device, *out_image, &mem_requirements);

    u32 mem_type_idx = -1;
    {
        VkPhysicalDeviceMemoryProperties mem_properties;
        vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_properties);
        for (u32 i = 0; i < mem_properties.memoryTypeCount; i++) {
            if ((mem_requirements.memoryTypeBits & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
                mem_type_idx = i;
                break;
            }
        }
        if (mem_type_idx == -1) {
            Panic("Failed to find memory type");
        }
    }

    VkMemoryAllocateInfo alloc_info = {
        .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize  = mem_requirements.size,
        .memoryTypeIndex = mem_type_idx,
    };

    VKExpect(vkAllocateMemory(device, &alloc_info, nullptr, out_image_memory));

    vkBindImageMemory(device, *out_image, *out_image_memory, 0);

    if (drop_pool) {
        drop_pool->push(vkDestroyImage, *out_image);
        drop_pool->push(vkFreeMemory, *out_image_memory);
    }
}

VkCommandBuffer Gfx::vk_one_shot_command_buffer_begin() {
    VkCommandBufferAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandPool        = command_pool,
        .commandBufferCount = 1,
    };

    VkCommandBuffer cmd_buffer;
    vkAllocateCommandBuffers(device, &alloc_info, &cmd_buffer);

    VkCommandBufferBeginInfo begin_info = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
    };
    vkBeginCommandBuffer(cmd_buffer, &begin_info);

    return cmd_buffer;
}

void Gfx::vk_one_shot_command_buffer_submit(VkCommandBuffer cmd_buffer) {
    vkEndCommandBuffer(cmd_buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &cmd_buffer;

    vkQueueSubmit(graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphics_queue);

    vkFreeCommandBuffers(device, command_pool, 1, &cmd_buffer);
}

void Gfx::vk_copy_buffer(VkBuffer dest, VkBuffer src, VkDeviceSize size) {
    VkCommandBuffer cmd_buffer = vk_one_shot_command_buffer_begin();

    VkBufferCopy copy_region = {.size = size};
    vkCmdCopyBuffer(cmd_buffer, src, dest, 1, &copy_region);

    vk_one_shot_command_buffer_submit(cmd_buffer);
}

void Gfx::vk_copy_buffer_to_image(VkImage dest, VkBuffer src, u32 width, u32 height) {
    VkCommandBuffer cmd_buffer = vk_one_shot_command_buffer_begin();

    VkBufferImageCopy region = {
        .bufferOffset                    = 0,
        .bufferRowLength                 = 0,
        .bufferImageHeight               = 0,
        .imageSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .imageSubresource.mipLevel       = 0,
        .imageSubresource.baseArrayLayer = 0,
        .imageSubresource.layerCount     = 1,
        .imageOffset                     = {0, 0, 0},
        .imageExtent                     = {width, height, 1},
    };
    vkCmdCopyBufferToImage(cmd_buffer, src, dest, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

    vk_one_shot_command_buffer_submit(cmd_buffer);
}

void Gfx::vk_transition_image_layout(VkImage image, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, u32 mip_levels) {
    VkCommandBuffer cmd_buffer = vk_one_shot_command_buffer_begin();

    VkImageMemoryBarrier barrier = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout                       = old_layout,
        .newLayout                       = new_layout,
        .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
        .image                           = image,
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = mip_levels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
    };

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        source_stage          = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destination_stage     = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        source_stage          = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destination_stage     = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        Panic("Unsupported layout transition");
    }

    vkCmdPipelineBarrier(cmd_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    vk_one_shot_command_buffer_submit(cmd_buffer);
}

// -----------------------------------------------------------------------------

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

// -----------------------------------------------------------------------------

VKDropPool VKDropPool::alloc(Arena* arena, usize capacity) {
    VKDropPool ret = {};
    ret.elems      = Vec<Element>::alloc(arena, capacity);
    return ret;
}

forall(T) void VKDropPool::push(void (*drop)(VkDevice, T*, const VkAllocationCallbacks*), T* target) {
    *elems.push() = Element{(DropFn)drop, target};
}

void VKDropPool::drop_all(VkDevice device) {
    for (isize i = (isize)elems.count - 1; i >= 0; --i) {
        elems.elems[i].drop(device, elems.elems[i].target, nullptr);
    }
    elems.count = 0;
}

// -----------------------------------------------------------------------------
}  // namespace