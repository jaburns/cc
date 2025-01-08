#pragma once
#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

struct PipelineSpec {
    Slice<u32> vert_spirv;
    Slice<u32> frag_spirv;

    Slice<VkVertexInputBindingDescription>     vertex_bindings;
    Slice<VkVertexInputAttributeDescription>   vertex_attributes;
    Slice<VkDescriptorSetLayout>               descriptor_set_layouts;
    Slice<VkPipelineColorBlendAttachmentState> color_blend_per_attachment;

    VkPipelineInputAssemblyStateCreateInfo input_assembly;
    VkPipelineRasterizationStateCreateInfo rasterizer;
    VkPipelineMultisampleStateCreateInfo   multisampling;
    VkPipelineColorBlendAttachmentState    color_blend_single_attachment;
    VkPipelineColorBlendStateCreateInfo    color_blend;
    VkPipelineDepthStencilStateCreateInfo  depth_stencil;

    static PipelineSpec mk_default();
};

struct PipelineInstance {
    VkPipelineLayout layout;
    VkPipeline       pipeline;

    static PipelineInstance create(VKDropPool* drop_pool, VkDevice device, VkRenderPass render_pass, PipelineSpec spec);
};

VkFormat                                 vk_get_format_for_type_name(Str type_name);
Slice<VkVertexInputAttributeDescription> vk_get_vertex_attributes_for_struct(Arena* arena, u32 binding, Slice<StructMemberInfo> struct_info);

// -----------------------------------------------------------------------------
}  // namespace
