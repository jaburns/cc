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

void GfxWindow::init(cchar* window_title, SDL_AudioCallback sdl_audio_callback) {
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

    ZeroStruct(this);
    auto scratch = Arena::create(memory_get_global_allocator(), 0);

    // init sdl
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
        // bool checkValidationLayerSupport() {
        //     uint32_t layerCount;
        //     vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        //    std::vector<VkLayerProperties> availableLayers(layerCount);
        //    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        //    for (const char* layerName : validationLayers) {
        //        bool layerFound = false;

        //        for (const auto& layerProperties : availableLayers) {
        //            if (strcmp(layerName, layerProperties.layerName) == 0) {
        //                layerFound = true;
        //                break;
        //            }
        //        }

        //        if (!layerFound) {
        //            return false;
        //        }
        //    }

        //    return true;
        //}

        auto extensions = VKGetSlice(vkEnumerateInstanceExtensionProperties, VkExtensionProperties, &scratch, nullptr);

        Assert(extensions.contains_all<cchar*>(
            instance_extensions.slice(),
            [](auto a, auto b) { return cstr_eq(a->extensionName, *b); }
        ));

        VkApplicationInfo app_info  = {};
        app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        app_info.pApplicationName   = window_title;
        app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        app_info.pEngineName        = "thirtythree";
        app_info.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
        app_info.apiVersion         = VK_API_VERSION_1_1;

        VkInstanceCreateInfo create_info    = {};
        create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        create_info.pApplicationInfo        = &app_info;
        create_info.enabledExtensionCount   = instance_extensions.count;
        create_info.ppEnabledExtensionNames = instance_extensions.elems;
        create_info.flags                   = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        if (ENABLE_VALIDATION_LAYERS) {
            create_info.enabledLayerCount   = 1;
            create_info.ppEnabledLayerNames = validation_layers.elems;

            VkDebugUtilsMessengerCreateInfoEXT debug_create_info{};
            debug_create_info.sType           = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            debug_create_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            debug_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            debug_create_info.pfnUserCallback = gfx_vulkan_debug_callback;

            create_info.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debug_create_info;
        } else {
            create_info.enabledLayerCount = 0;
            create_info.pNext             = nullptr;
        }

        VKCall(vkCreateInstance(&create_info, nullptr, &instance), "Failed to create Vulkan instance");
        println("Vulkan instance created successfully");

        if (!SDL_Vulkan_CreateSurface(sdl_window, instance, &surface)) Panic("Failed to create Vulkan surface: %s\n", SDL_GetError());
    }
    // create device
    VkSurfaceCapabilitiesKHR  surface_capabilities = {};
    Slice<VkSurfaceFormatKHR> surface_formats      = {};
    Slice<VkPresentModeKHR>   present_modes        = {};
    {
        auto physical_devices = VKGetSlice(vkEnumeratePhysicalDevices, VkPhysicalDevice, &scratch, instance);
        if (physical_devices.count == 0) Panic("Failed to find GPUs with Vulkan support");

        i32 graphics_queue_idx;
        i32 present_queue_idx;
        for (u32 i = 0; i < physical_devices.count; ++i) {
            physical_device    = physical_devices.elems[i];
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
            present_modes = VKGetSlice(vkGetPhysicalDeviceSurfacePresentModesKHR, VkPresentModeKHR, &scratch, physical_device, surface);
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
        if (graphics_queue_idx < 0) Panic("Failed to find a suitable GPU!");

        VkDeviceQueueCreateInfo present_queue_create_info{};
        present_queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        present_queue_create_info.queueFamilyIndex = (u32)present_queue_idx;
        present_queue_create_info.queueCount       = 1;
        present_queue_create_info.pQueuePriorities = &queue_priority;

        VkDeviceQueueCreateInfo gfx_queue_create_info{};
        gfx_queue_create_info.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        gfx_queue_create_info.queueFamilyIndex = (u32)graphics_queue_idx;
        gfx_queue_create_info.queueCount       = 1;
        gfx_queue_create_info.pQueuePriorities = &queue_priority;

        VkPhysicalDeviceFeatures device_features{};
        VkDeviceCreateInfo       create_info{};

        VkDeviceQueueCreateInfo infoz[] = {
            gfx_queue_create_info,
            present_queue_create_info,
        };
        create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        create_info.pQueueCreateInfos       = infoz;
        create_info.queueCreateInfoCount    = present_queue_idx == graphics_queue_idx ? 1 : 2;  // queuecreateinfo's queueFamilyIndexs must be unique
        create_info.pEnabledFeatures        = &device_features;
        create_info.enabledExtensionCount   = device_extensions.count;
        create_info.ppEnabledExtensionNames = device_extensions.elems;

        if (ENABLE_VALIDATION_LAYERS) {
            create_info.enabledLayerCount   = 1;
            create_info.ppEnabledLayerNames = validation_layers.elems;
        } else {
            create_info.enabledLayerCount = 0;
        }

        VKCall(vkCreateDevice(physical_device, &create_info, nullptr, &device), "Failed to create logical device");

        vkGetDeviceQueue(device, graphics_queue_idx, 0, &graphics_queue);
        vkGetDeviceQueue(device, present_queue_idx, 0, &present_queue);
    }

    // static VkExtent2D chooseSwapExtent(SDL_Window* window, const VkSurfaceCapabilitiesKHR& capabilities) {
    //     if (capabilities.currentExtent.width != UINT32_MAX) {
    //         return capabilities.currentExtent;
    //     } else {
    //         int width, height;
    //         SDL_Vulkan_GetDrawableSize(window, &width, &height);
    //
    //         VkExtent2D actualExtent = {
    //             static_cast<uint32_t>(width),
    //             static_cast<uint32_t>(height)
    //         };
    //
    //         actualExtent.width  = clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    //         actualExtent.height = clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
    //
    //         return actualExtent;
    //     }
    // }
    //
    // static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
    //     for (const auto& availableFormat : availableFormats) {
    //         if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
    //             return availableFormat;
    //         }
    //     }
    //
    //     return availableFormats[0];
    // }
    // {
    //     SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physical_device, surface);

    //     VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    //     VkPresentModeKHR   presentMode   = VK_PRESENT_MODE_FIFO_KHR;
    //     VkExtent2D         extent        = chooseSwapExtent(sdl_window, swapChainSupport.capabilities);

    //     uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    //     if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
    //         imageCount = swapChainSupport.capabilities.maxImageCount;
    //     }

    //     VkSwapchainCreateInfoKHR createInfo{};
    //     createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    //     createInfo.surface          = surface;
    //     createInfo.minImageCount    = imageCount;
    //     createInfo.imageFormat      = surfaceFormat.format;
    //     createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    //     createInfo.imageExtent      = extent;
    //     createInfo.imageArrayLayers = 1;
    //     createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    //     createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    //     createInfo.preTransform     = swapChainSupport.capabilities.currentTransform;
    //     createInfo.compositeAlpha   = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    //     createInfo.presentMode      = presentMode;
    //     createInfo.clipped          = VK_TRUE;
    //     createInfo.oldSwapchain     = VK_NULL_HANDLE;

    //     if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swap_chain) != VK_SUCCESS) {
    //         Panic("failed to create swap chain!");
    //     }

    //     swap_chain_image_format = surfaceFormat.format;
    //     swap_chain_extent       = extent;
    // }
    // TODO continue from here - https://vulkan-tutorial.com/Drawing_a_triangle/Presentation/Swap_chain

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
}

}  // namespace