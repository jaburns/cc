#include "inc.hh"
namespace {

struct Vertex {
    vec2 pos;
    vec3 color;

    static VkVertexInputBindingDescription get_binding_desc() {
        return VkVertexInputBindingDescription{
            .binding   = 0,
            .stride    = sizeof(Vertex),
            .inputRate = VK_VERTEX_INPUT_RATE_VERTEX,
        };
    }

    static Array<VkVertexInputAttributeDescription, 2> get_attribute_descs() {
        return Array<VkVertexInputAttributeDescription, 2>{{
            {
                .binding  = 0,
                .location = 0,
                .format   = VK_FORMAT_R32G32_SFLOAT,
                .offset   = offsetof(Vertex, pos),
            },
            {
                .binding  = 0,
                .location = 1,
                .format   = VK_FORMAT_R32G32B32_SFLOAT,
                .offset   = offsetof(Vertex, color),
            },
        }};
    }
};

Array<Vertex, 4> vertices = {{
    {vec2(-0.5f, -0.5f), vec3(1.0f, 0.0f, 0.0f)},
    {vec2(0.5f, -0.5f), vec3(0.0f, 1.0f, 0.0f)},
    {vec2(0.5f, 0.5f), vec3(0.0f, 0.0f, 1.0f)},
    {vec2(-0.5f, 0.5f), vec3(1.0f, 1.0f, 1.0f)},
}};

Array<u16, 6> indices = {{0, 1, 2, 2, 3, 0}};

void GfxApp::init(GfxWindow* gfx) {
    ZeroStruct(this);
    auto scratch = Arena::create(memory_get_global_allocator(), 0);
    {
        Slice<u8> vert_code   = fs_read_file_bytes(&scratch, "shaders/bin/triangle.vertex.spv");
        auto      vert_module = VkShaderModule{};

        auto vert_create_info = VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = vert_code.count,
            .pCode    = (u32*)vert_code.elems,
        };
        VKExpect(vkCreateShaderModule(gfx->device, &vert_create_info, nullptr, &vert_module));

        Slice<u8> frag_code   = fs_read_file_bytes(&scratch, "shaders/bin/triangle.fragment.spv");
        auto      frag_module = VkShaderModule{};

        auto frag_create_info = VkShaderModuleCreateInfo{
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = frag_code.count,
            .pCode    = (u32*)frag_code.elems,
        };
        VKExpect(vkCreateShaderModule(gfx->device, &frag_create_info, nullptr, &frag_module));

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

        auto vertex_binding_desc    = Vertex::get_binding_desc();
        auto vertex_attribute_descs = Vertex::get_attribute_descs();

        auto vertex_input_info = VkPipelineVertexInputStateCreateInfo{
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = 1,
            .pVertexBindingDescriptions      = &vertex_binding_desc,
            .vertexAttributeDescriptionCount = (u32)vertex_attribute_descs.count,
            .pVertexAttributeDescriptions    = vertex_attribute_descs.elems,
        };
        auto input_assembly = VkPipelineInputAssemblyStateCreateInfo{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
            .primitiveRestartEnable = VK_FALSE,
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
        auto rasterizer = VkPipelineRasterizationStateCreateInfo{
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = VK_POLYGON_MODE_FILL,
            .lineWidth               = 1.f,
            .cullMode                = VK_CULL_MODE_BACK_BIT,
            .frontFace               = VK_FRONT_FACE_CLOCKWISE,
            .depthBiasEnable         = VK_FALSE,
        };
        auto multisampling = VkPipelineMultisampleStateCreateInfo{
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .sampleShadingEnable  = VK_FALSE,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        };
        auto color_blend_attachment = VkPipelineColorBlendAttachmentState{
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            .blendEnable    = VK_FALSE,
        };
        auto color_blending = VkPipelineColorBlendStateCreateInfo{
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &color_blend_attachment,
        };

        auto pipeline_layout_info = VkPipelineLayoutCreateInfo{
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = 0,
            .pSetLayouts            = nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };
        VkPipelineLayout pipeline_layout;
        VKExpect(vkCreatePipelineLayout(gfx->device, &pipeline_layout_info, nullptr, &pipeline_layout));

        auto pipeline_info = VkGraphicsPipelineCreateInfo{
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = 2,
            .pStages             = shader_stages,
            .pVertexInputState   = &vertex_input_info,
            .pInputAssemblyState = &input_assembly,
            .pDynamicState       = &dynamic_state,
            .pViewportState      = &viewport_state,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = nullptr,
            .pColorBlendState    = &color_blending,
            .layout              = pipeline_layout,
            .renderPass          = gfx->main_pass,
            .subpass             = 0,
        };
        VKExpect(vkCreateGraphicsPipelines(gfx->device, nullptr, 1, &pipeline_info, nullptr, &gfx_pipeline));

        vkDestroyShaderModule(gfx->device, frag_module, nullptr);
        vkDestroyShaderModule(gfx->device, vert_module, nullptr);

        // vertex buffer
        {
            VkDeviceSize size = sizeof(Vertex) * vertices.count;

            VkBuffer       staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            gfx->vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

            void* data;
            vkMapMemory(gfx->device, staging_buffer_memory, 0, size, 0, &data);
            memcpy(data, vertices.elems, size);
            vkUnmapMemory(gfx->device, staging_buffer_memory);

            gfx->vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &vertex_buffer, &vertex_buffer_memory);

            gfx->vk_copy_buffer(gfx->graphics_queue, vertex_buffer, staging_buffer, size);

            vkDestroyBuffer(gfx->device, staging_buffer, nullptr);
            vkFreeMemory(gfx->device, staging_buffer_memory, nullptr);
        }
        // index buffer
        {
            VkDeviceSize size = sizeof(u16) * indices.count;

            VkBuffer       staging_buffer;
            VkDeviceMemory staging_buffer_memory;

            gfx->vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &staging_buffer, &staging_buffer_memory);

            void* data;
            vkMapMemory(gfx->device, staging_buffer_memory, 0, size, 0, &data);
            memcpy(data, indices.elems, size);
            vkUnmapMemory(gfx->device, staging_buffer_memory);

            gfx->vk_create_buffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &index_buffer, &index_buffer_memory);

            gfx->vk_copy_buffer(gfx->graphics_queue, index_buffer, staging_buffer, size);

            vkDestroyBuffer(gfx->device, staging_buffer, nullptr);
            vkFreeMemory(gfx->device, staging_buffer_memory, nullptr);
        }
    }

    scratch.destroy();
}

void GfxApp::frame(GfxFrameContext ctx) {
    auto clear_color = VkClearValue{{{0.0f, 0.0f, 0.0f, 1.0f}}};

    auto render_pass_info = VkRenderPassBeginInfo{
        .sType             = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass        = ctx.main_pass,
        .framebuffer       = ctx.main_pass_framebuffer,
        .renderArea.offset = {0, 0},
        .renderArea.extent = ctx.main_pass_extent,
        .clearValueCount   = 1,
        .pClearValues      = &clear_color,
    };
    vkCmdBeginRenderPass(ctx.buffer, &render_pass_info, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(ctx.buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfx_pipeline);

    auto viewport = VkViewport{
        .x        = 0.0f,
        .y        = 0.0f,
        .width    = (f32)ctx.main_pass_extent.width,
        .height   = (f32)ctx.main_pass_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };
    vkCmdSetViewport(ctx.buffer, 0, 1, &viewport);

    auto scissor = VkRect2D{
        .offset = {0, 0},
        .extent = ctx.main_pass_extent,
    };
    vkCmdSetScissor(ctx.buffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(ctx.buffer, 0, 1, &vertex_buffer, offsets);
    vkCmdBindIndexBuffer(ctx.buffer, index_buffer, 0, VK_INDEX_TYPE_UINT16);
    vkCmdDrawIndexed(ctx.buffer, (u32)indices.count, 1, 0, 0, 0);

    vkCmdEndRenderPass(ctx.buffer);
}

}  // namespace