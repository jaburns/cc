#pragma once
#include "inc.hh"
namespace {

struct Texture {
    VkImage        image;
    VkImageView    image_view;
    VkDeviceMemory memory;
    VkSampler      sampler;

    static Texture create_from_png(Gfx* gfx, VKDropPool* drop_pool, cchar* path);
    static Texture create(Gfx* gfx, VKDropPool* drop_pool, u8* rgba, u32 width, u32 height);
};

}  // namespace
