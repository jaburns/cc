#pragma once

struct GfxWindow {
    SDL_Window*      sdl_window;
    VkInstance       instance;
    VkPhysicalDevice physicalDevice;
    VkDevice         device;
    VkQueue          graphicsQueue;
    VkQueue          presentQueue;
    VkSurfaceKHR     surface;
    VkSwapchainKHR   swapChain;
    Slice<VkImage>   swapChainImages;
    VkFormat         swapChainImageFormat;
    VkExtent2D       swapChainExtent;
};