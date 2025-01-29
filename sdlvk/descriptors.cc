#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

forall(T) PerSwap<T*> DescriptorSetBuilder::add_uniform_buffer_object(VkShaderStageFlags stage_flags) {
    Ubo* ubo = ubos.push();

    ubo->size        = sizeof(T);
    ubo->stage_flags = stage_flags;

    PerSwap<T*> mapped = {};

    for (usize i = 0; i < GFX_MAX_FRAMES_IN_FLIGHT; ++i) {
        gfx->vk_create_buffer(
            sizeof(T),
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            drop_pool, &ubo->buffer[i], &ubo->memory[i]
        );
        vkMapMemory(gfx->device, ubo->memory[i], 0, sizeof(T), 0, (void**)&mapped[i]);
    }

    return mapped;
}

void DescriptorSetBuilder::add_combined_image_sampler(Texture* texture) {
    *textures.push() = texture;
}

DescriptorSet DescriptorSetBuilder::build() {
    constexpr usize VEC_CAPACITY = 64;
    ScratchArena    scratch{};

    DescriptorSet ret = {};

    auto bindings          = Vec<VkDescriptorSetLayoutBinding>::alloc(scratch.arena, VEC_CAPACITY);
    auto pool_sizes        = Vec<VkDescriptorPoolSize>::alloc(scratch.arena, VEC_CAPACITY);
    auto buffer_infos      = Vec<VkDescriptorBufferInfo>::alloc(scratch.arena, VEC_CAPACITY);
    auto image_infos       = Vec<VkDescriptorImageInfo>::alloc(scratch.arena, VEC_CAPACITY);
    auto descriptor_writes = Vec<VkWriteDescriptorSet>::alloc(scratch.arena, VEC_CAPACITY);

    for (u32 i = 0; i < ubos.count; ++i) {
        *bindings.push() = {
            .binding            = i,
            .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount    = 1,
            .stageFlags         = ubos.elems[i].stage_flags,
            .pImmutableSamplers = nullptr,
        };
    }
    for (u32 i = 0; i < textures.count; ++i) {
        *bindings.push() = {
            .binding            = (u32)ubos.count + i,
            .descriptorCount    = 1,
            .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
            .pImmutableSamplers = nullptr,
        };
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = (u32)bindings.count,
        .pBindings    = bindings.elems,
    };
    VKExpect(vkCreateDescriptorSetLayout(gfx->device, &layout_info, nullptr, &ret.layout));
    drop_pool->push(vkDestroyDescriptorSetLayout, ret.layout);

    for (u32 i = 0; i < ubos.count; ++i) {
        *pool_sizes.push() = {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
        };
    }
    for (u32 i = 0; i < textures.count; ++i) {
        *pool_sizes.push() = {
            .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
        };
    }

    VkDescriptorPool descriptor_pool;

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = (u32)pool_sizes.count,
        .pPoolSizes    = pool_sizes.elems,
        .maxSets       = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
    };
    VKExpect(vkCreateDescriptorPool(gfx->device, &pool_info, nullptr, &descriptor_pool));
    drop_pool->push(vkDestroyDescriptorPool, descriptor_pool);

    PerSwap<VkDescriptorSetLayout> layouts = {};
    layouts.slice().fill_copy(&ret.layout);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptor_pool,
        .descriptorSetCount = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts        = layouts.elems,
    };
    VKExpect(vkAllocateDescriptorSets(gfx->device, &alloc_info, ret.set.elems));

    for (u32 j = 0; j < GFX_MAX_FRAMES_IN_FLIGHT; j++) {
        descriptor_writes.count = 0;

        for (u32 i = 0; i < ubos.count; ++i) {
            *buffer_infos.push() = {
                .buffer = ubos.elems[i].buffer[j],
                .offset = 0,
                .range  = ubos.elems[i].size,
            };
            *descriptor_writes.push() = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = ret.set[j],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo     = &buffer_infos.elems[buffer_infos.count - 1],
            };
        }

        for (u32 i = 0; i < textures.count; ++i) {
            *image_infos.push() = {
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                .imageView   = textures.elems[i]->image_view,
                .sampler     = textures.elems[i]->sampler,
            };
            *descriptor_writes.push() = {
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = ret.set[j],
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo      = &image_infos.elems[image_infos.count - 1],
            };
        }

        vkUpdateDescriptorSets(gfx->device, (u32)descriptor_writes.count, descriptor_writes.elems, 0, nullptr);
    }

    return ret;
}

// -----------------------------------------------------------------------------
}  // namespace