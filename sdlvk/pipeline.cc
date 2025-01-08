#include "inc.hh"
namespace {
// -----------------------------------------------------------------------------

PipelineSpec PipelineSpec::mk_default() {
    return {
        .input_assembly = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
        },
        .rasterizer = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .lineWidth               = 1.f,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_COUNTER_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
        },
        .multisampling = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable  = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        },
        .color_blend_single_attachment = {
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable    = VK_FALSE,
        },
        .color_blend = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO, .logicOpEnable = VK_FALSE, .logicOp = VK_LOGIC_OP_COPY,
            .attachmentCount = 0,        // overwritten on instantiation
            .pAttachments    = nullptr,  // overwritten on instantiation
        },
        .depth_stencil = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = VK_TRUE,
            .depthWriteEnable      = VK_FALSE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
        }
    };
}

// -----------------------------------------------------------------------------

PipelineInstance PipelineInstance::create(VKDropPool* drop_pool, VkDevice device, VkRenderPass render_pass, PipelineSpec spec) {
    auto vert_create_info = VkShaderModuleCreateInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(u32) * spec.vert_spirv.count,
        .pCode    = spec.vert_spirv.elems,
    };
    VkShaderModule vert_module;
    VKExpect(vkCreateShaderModule(device, &vert_create_info, nullptr, &vert_module));
    defer { vkDestroyShaderModule(device, vert_module, nullptr); };

    auto frag_create_info = VkShaderModuleCreateInfo{
        .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = sizeof(u32) * spec.frag_spirv.count,
        .pCode    = spec.frag_spirv.elems,
    };
    VkShaderModule frag_module;
    VKExpect(vkCreateShaderModule(device, &frag_create_info, nullptr, &frag_module));
    defer { vkDestroyShaderModule(device, frag_module, nullptr); };

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

    auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount   = (u32)spec.vertex_bindings.count,
        .pVertexBindingDescriptions      = spec.vertex_bindings.elems,
        .vertexAttributeDescriptionCount = (u32)spec.vertex_attributes.count,
        .pVertexAttributeDescriptions    = spec.vertex_attributes.elems,
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

    PipelineInstance ret;

    auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount         = (u32)spec.descriptor_set_layouts.count,
        .pSetLayouts            = spec.descriptor_set_layouts.elems,
        .pushConstantRangeCount = 0,
    };
    VKExpect(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &ret.layout));
    drop_pool->push(vkDestroyPipelineLayout, ret.layout);

    if (spec.color_blend_per_attachment.count) {
        spec.color_blend.attachmentCount = (u32)spec.color_blend_per_attachment.count;
        spec.color_blend.pAttachments    = spec.color_blend_per_attachment.elems;
    } else {
        spec.color_blend.attachmentCount = 1;
        spec.color_blend.pAttachments    = &spec.color_blend_single_attachment;
    }

    auto pipeline_info = VkGraphicsPipelineCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount          = 2,
        .pStages             = shader_stages,
        .pVertexInputState   = &vertex_input_info,
        .pInputAssemblyState = &spec.input_assembly,
        .pDynamicState       = &dynamic_state,
        .pViewportState      = &viewport_state,
        .pRasterizationState = &spec.rasterizer,
        .pMultisampleState   = &spec.multisampling,
        .pDepthStencilState  = &spec.depth_stencil,
        .pColorBlendState    = &spec.color_blend,
        .layout              = ret.layout,
        .renderPass          = render_pass,
        .subpass             = 0,
    };
    VKExpect(vkCreateGraphicsPipelines(device, nullptr, 1, &pipeline_info, nullptr, &ret.pipeline));
    drop_pool->push(vkDestroyPipeline, ret.pipeline);

    return ret;
}

// -----------------------------------------------------------------------------
}  // namespace