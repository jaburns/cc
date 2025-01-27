#pragma once
#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

struct DescriptorSet {
    VkDescriptorSetLayout    layout;
    PerSwap<VkDescriptorSet> set;
};

class DescriptorSetBuilder {
    Gfx*        gfx;
    VKDropPool* drop_pool;

    struct Ubo {
        usize                   size;
        PerSwap<VkBuffer>       buffer;
        PerSwap<VkDeviceMemory> memory;
    };

    InlineVec<Ubo, 32>      ubos;
    InlineVec<Texture*, 32> textures;

  public:
    DescriptorSetBuilder(Gfx* gfx, VKDropPool* drop_pool) : gfx(gfx),
                                                            drop_pool(drop_pool),
                                                            ubos({}),
                                                            textures({}) {}

    forall(T) PerSwap<T*> add_uniform_buffer_object();
    void add_combined_image_sampler(Texture* texture);

    DescriptorSet build();
};

// -----------------------------------------------------------------------------
}  // namespace