#include "inc.hh"
namespace {

Texture Texture::create_from_png(Gfx* gfx, VKDropPool* drop_pool, cchar* path) {
    i32     width, height, channels;
    u8*     rgba = stbi_load("assets/textures/room.png", &width, &height, &channels, STBI_rgb_alpha);
    Texture ret  = create(gfx, drop_pool, rgba, (u32)width, (u32)height);
    stbi_image_free(rgba);
    return ret;
}

Texture Texture::create(Gfx* gfx, VKDropPool* drop_pool, u8* rgba, u32 width, u32 height) {
    ScratchArena scratch{};
    VKDropPool   vk_scratch = VKDropPool::alloc(scratch.arena, 32);
    defer { vk_scratch.drop_all(gfx->device); };

    Texture ret = {};

    u32 mip_levels = 1 + (u32)(floorf(log2f((f32)max(width, height))));

    VkDeviceSize   size = width * height * 4;
    VkBuffer       staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    gfx->vk_create_buffer(
        size,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        &vk_scratch, &staging_buffer, &staging_buffer_memory
    );

    void* data;
    vkMapMemory(gfx->device, staging_buffer_memory, 0, size, 0, &data);
    CopyArray(data, rgba, size);
    vkUnmapMemory(gfx->device, staging_buffer_memory);

    gfx->vk_create_image(
        width, height, mip_levels, VK_SAMPLE_COUNT_1_BIT,
        VK_FORMAT_R8G8B8A8_UNORM,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        drop_pool, &ret.image, &ret.memory
    );

    gfx->vk_transition_image_layout(ret.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
    gfx->vk_copy_buffer_to_image(ret.image, staging_buffer, width, height);

    VkImageViewCreateInfo view_info = {
        .sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image                           = ret.image,
        .viewType                        = VK_IMAGE_VIEW_TYPE_2D,
        .format                          = VK_FORMAT_R8G8B8A8_UNORM,
        .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
        .subresourceRange.baseMipLevel   = 0,
        .subresourceRange.levelCount     = mip_levels,
        .subresourceRange.baseArrayLayer = 0,
        .subresourceRange.layerCount     = 1,
    };
    VKExpect(vkCreateImageView(gfx->device, &view_info, nullptr, &ret.image_view));
    drop_pool->push(vkDestroyImageView, ret.image_view);

    VkPhysicalDeviceProperties properties;
    vkGetPhysicalDeviceProperties(gfx->physical_device, &properties);

    VkSamplerCreateInfo sampler_info = {
        .sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter               = VK_FILTER_LINEAR,
        .minFilter               = VK_FILTER_LINEAR,
        .addressModeU            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeV            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .addressModeW            = VK_SAMPLER_ADDRESS_MODE_REPEAT,
        .anisotropyEnable        = VK_TRUE,
        .maxAnisotropy           = properties.limits.maxSamplerAnisotropy,
        .borderColor             = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
        .unnormalizedCoordinates = VK_FALSE,
        .compareEnable           = VK_FALSE,
        .compareOp               = VK_COMPARE_OP_ALWAYS,
        .mipmapMode              = VK_SAMPLER_MIPMAP_MODE_LINEAR,
        .minLod                  = 0.f,
        .maxLod                  = (f32)mip_levels,
    };
    VKExpect(vkCreateSampler(gfx->device, &sampler_info, nullptr, &ret.sampler));

    {
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(gfx->physical_device, VK_FORMAT_R8G8B8A8_UNORM, &formatProperties);
        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            Panic("Texture image format does not support linear blitting");
        }

        VkCommandBuffer commandBuffer = gfx->vk_one_shot_command_buffer_begin();

        VkImageMemoryBarrier barrier = {
            .sType                           = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .image                           = ret.image,
            .srcQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex             = VK_QUEUE_FAMILY_IGNORED,
            .subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
            .subresourceRange.baseArrayLayer = 0,
            .subresourceRange.layerCount     = 1,
            .subresourceRange.levelCount     = 1,
        };

        i32 mip_width  = width;
        i32 mip_height = height;

        for (u32 i = 1; i < mip_levels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout                     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask                 = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            VkImageBlit blit = {
                .srcOffsets                    = {{0, 0, 0}, {mip_width, mip_height, 1}},
                .srcSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .srcSubresource.mipLevel       = i - 1,
                .srcSubresource.baseArrayLayer = 0,
                .srcSubresource.layerCount     = 1,
                .dstOffsets                    = {{0, 0, 0}, {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1}},
                .dstSubresource.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                .dstSubresource.mipLevel       = i,
                .dstSubresource.baseArrayLayer = 0,
                .dstSubresource.layerCount     = 1,
            };

            vkCmdBlitImage(commandBuffer, ret.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ret.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);

            barrier.oldLayout     = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            if (mip_width > 1) mip_width /= 2;
            if (mip_height > 1) mip_height /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mip_levels - 1;
        barrier.oldLayout                     = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout                     = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask                 = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask                 = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        gfx->vk_one_shot_command_buffer_submit(commandBuffer);
    }

    return ret;
}

}  // namespace