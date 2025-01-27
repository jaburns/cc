#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

forall(T) PerSwap<T*> DescriptorSetBuilder::add_uniform_buffer_object() {
    Ubo* ubo = ubos.push();

    ubo->size = sizeof(T);

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
    DescriptorSet ret = {};

    VkDescriptorSetLayoutBinding ubo_layout_binding = {
        .binding            = 0,
        .descriptorType     = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount    = 1,
        .stageFlags         = VK_SHADER_STAGE_VERTEX_BIT,
        .pImmutableSamplers = nullptr,
    };
    VkDescriptorSetLayoutBinding sampler_layout_binding = {
        .binding            = 1,
        .descriptorCount    = 1,
        .descriptorType     = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .stageFlags         = VK_SHADER_STAGE_FRAGMENT_BIT,
        .pImmutableSamplers = nullptr,
    };

    VkDescriptorSetLayoutBinding bindings[] = {
        ubo_layout_binding,
        sampler_layout_binding,
    };

    VkDescriptorSetLayoutCreateInfo layout_info = {
        .sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = RawArrayLen(bindings),
        .pBindings    = bindings,
    };
    VKExpect(vkCreateDescriptorSetLayout(gfx->device, &layout_info, nullptr, &ret.layout));
    drop_pool->push(vkDestroyDescriptorSetLayout, ret.layout);

    VkDescriptorPoolSize pool_sizes[] = {
        VkDescriptorPoolSize{
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
        },
        VkDescriptorPoolSize{
            .type            = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .descriptorCount = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
        },
    };

    VkDescriptorPool descriptor_pool;

    VkDescriptorPoolCreateInfo pool_info = {
        .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .poolSizeCount = RawArrayLen(pool_sizes),
        .pPoolSizes    = pool_sizes,
        .maxSets       = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
    };
    VKExpect(vkCreateDescriptorPool(gfx->device, &pool_info, nullptr, &descriptor_pool));
    drop_pool->push(vkDestroyDescriptorPool, descriptor_pool);

    Array<VkDescriptorSetLayout, GFX_MAX_FRAMES_IN_FLIGHT> layouts = {};
    layouts.slice().fill_copy(&ret.layout);

    VkDescriptorSetAllocateInfo alloc_info = {
        .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool     = descriptor_pool,
        .descriptorSetCount = (u32)GFX_MAX_FRAMES_IN_FLIGHT,
        .pSetLayouts        = layouts.elems,
    };
    VKExpect(vkAllocateDescriptorSets(gfx->device, &alloc_info, ret.set.elems));

    for (usize i = 0; i < GFX_MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo buffer_info = {
            .buffer = ubos.elems[0].buffer[i],
            .offset = 0,
            .range  = ubos.elems[0].size,
        };
        VkDescriptorImageInfo image_info = {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            .imageView   = textures.elems[0]->image_view,
            .sampler     = textures.elems[0]->sampler,
        };

        VkWriteDescriptorSet descriptor_writes[] = {
            VkWriteDescriptorSet{
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = ret.set[i],
                .dstBinding      = 0,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                .descriptorCount = 1,
                .pBufferInfo     = &buffer_info,
            },
            VkWriteDescriptorSet{
                .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet          = ret.set[i],
                .dstBinding      = 1,
                .dstArrayElement = 0,
                .descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
                .descriptorCount = 1,
                .pImageInfo      = &image_info,
            },
        };
        vkUpdateDescriptorSets(gfx->device, RawArrayLen(descriptor_writes), descriptor_writes, 0, nullptr);
    }

    return ret;
}

// -----------------------------------------------------------------------------
}  // namespace