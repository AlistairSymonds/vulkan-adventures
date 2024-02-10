#include "RasterEngine.h"
#include "RasterEngine.h"
#include "RasterEngine.h"
#include <vulkan/vulkan.h>
#include "RasterEngine.h"
#include "PipelineBuilder.h"
#include "vk_initializers.h"

RasterEngine::RasterEngine(VkDevice dev, VkInstance inst, VkExtent2D windowExt, std::shared_ptr<AssetManager> manager, VmaAllocator vmalloc) {
	am = manager;
	device = dev;
    instance = inst;
    windowExtent = windowExt;
    allocator = vmalloc;
}

std::string RasterEngine::getName()
{
	return "Raster";
}

RasterEngine::~RasterEngine()
{
}

void RasterEngine::cleanup() {
    cleanup_queue.flush();
}

void RasterEngine::init()
{
    pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
    init_output();
    init_draw_barriers();
    init_dynamic_rendering();
    init_pipelines();
    init_materials();
}


void RasterEngine::Draw(VkCommandBuffer cmdBuffer, RenderState state, std::vector<RenderObject> renderObjs)
{


    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &pre_draw_image_memory_barrier // pImageMemoryBarriers
    );
    vkCmdBeginRendering(cmdBuffer, &default_ri);

    MeshPushConstants skyboxconstants = {};
    skyboxconstants.render_matrix = state.camView;
    Material skybox_mat = get_material("skybox");
    vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skybox_mat.pipe);
    vkCmdPushConstants(cmdBuffer, skybox_mat.layout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(MeshPushConstants), &skyboxconstants);
    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);

    glm::mat4 viewProj = state.camProj * state.camView;
    for (auto &o : renderObjs)
    {
        Material m = get_material(o.materialId);
        glm::mat4 mvp = viewProj * o.worldMatrix;

        VkDeviceSize offset = 0;
        Mesh mesh = am->getMesh(o.meshId);
        if (mesh.buffer.indexBuffer.buf == nullptr)
            std::cout << "null idx" << std::endl;

        MeshPushConstants constants;
        constants.render_matrix = mvp;
        constants.vb = mesh.buffer.vertexBufferAddress;
        vkCmdBindPipeline(cmdBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m.pipe);
        vkCmdPushConstants(cmdBuffer, m.layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(MeshPushConstants), &constants);

        vkCmdBindIndexBuffer(cmdBuffer, mesh.buffer.indexBuffer.buf, 0, VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(cmdBuffer, am->getMesh(o.meshId).indices.size(), 1, 0, 0, 0);
    }
        
    vkCmdEndRendering(cmdBuffer);

    
    vkCmdPipelineBarrier(
        cmdBuffer,
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,  // srcStageMask
        VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT, // dstStageMask
        0,
        0,
        nullptr,
        0,
        nullptr,
        1, // imageMemoryBarrierCount
        &post_draw_image_memory_barrier // pImageMemoryBarriers
    );

}

VkFormat RasterEngine::getOutputFormat()
{
    return outputFormat;
}

VkImage RasterEngine::getOutputImage()
{

    return outputImage;
}

void RasterEngine::init_dynamic_rendering()
{
    default_colour_attach_info = {};
    default_colour_attach_info.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
    default_colour_attach_info.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    default_colour_attach_info.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    default_colour_attach_info.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    default_colour_attach_info.imageView = outputImageView;

    default_colour_attach_info.clearValue.color.float32[0] = 1.0;
    default_colour_attach_info.clearValue.color.float32[1] = 1.0;
    default_colour_attach_info.clearValue.color.float32[2] = 0.4;
    default_colour_attach_info.clearValue.color.float32[3] = 1.0;

    default_ri = {};
    default_ri.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    default_ri.colorAttachmentCount = 1;
    default_ri.layerCount = 1;
    default_ri.pColorAttachments = &default_colour_attach_info;
    default_ri.renderArea.extent = windowExtent;

}

void RasterEngine::init_pipelines()
{
    std::vector<VkPipeline> pipes;
    std::vector<VkPipelineLayout> layouts;

    VkPipelineLayoutCreateInfo pipeline_layout_info = vkinit::pipeline_layout_create_info();
    //use the layout then shove in the tripipe's layout
    VkPipelineLayout tri_pipe_layout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipeline_layout_info, nullptr, &tri_pipe_layout));
    layouts.push_back(tri_pipe_layout);

    PipelineBuilder pb;
    pb._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, am->getShaderModule("basic.vert"))
    );

    pb._shaderStages.push_back(
        vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, am->getShaderModule("basic.frag"))
    );

    pb._vertexInputInfo = vkinit::vertx_input_state_create_info();
    pb._inputAssembly = vkinit::input_assembly_create_info(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pb._viewport.x = 0.0f;
    pb._viewport.y = 0.0f;
    pb._viewport.width = (float)windowExtent.width;
    pb._viewport.height = (float)windowExtent.height;
    pb._viewport.minDepth = 0.0f;
    pb._viewport.maxDepth = 1.0f;

    pb._scissor.offset = { 0, 0 };
    pb._scissor.extent = windowExtent;


    pb._rasterizer = vkinit::raster_state_create_info();
    pb._multisampling = vkinit::multisampling_state_create_info();
    pb._colorBlendAttachment = vkinit::color_blend_attachment_state();

    pb._pipelineLayout = tri_pipe_layout;

    VkPipelineRenderingCreateInfo pipeline_rendering_create_info{
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .colorAttachmentCount = 1,
    .pColorAttachmentFormats = &outputFormat,
    };

    auto trianglePipe = pb.build_pipeline(device, pipeline_rendering_create_info);
    pipes.push_back(trianglePipe);

    //Now build mesh pipeline
    VkPipelineLayoutCreateInfo mesh_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    VkPushConstantRange push_constant;
    push_constant.size = sizeof(MeshPushConstants);
    push_constant.offset = 0;
    push_constant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mesh_pipeline_layout_info.pPushConstantRanges = &push_constant;
    mesh_pipeline_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout mesh_pipe_layout;
    VK_CHECK(vkCreatePipelineLayout(device, &mesh_pipeline_layout_info, nullptr, &mesh_pipe_layout));
    pb._pipelineLayout = mesh_pipe_layout;
    layouts.push_back(mesh_pipe_layout);

    auto vertexDesc = Vertex::get_vertex_description();
    pb._vertexInputInfo.pVertexAttributeDescriptions = 0;
    pb._vertexInputInfo.vertexAttributeDescriptionCount = 0;


    //pb._vertexInputInfo.pVertexBindingDescriptions = vertexDesc.bindings.data();
    //pb._vertexInputInfo.vertexBindingDescriptionCount = vertexDesc.bindings.size();
    pb._shaderStages.clear();
    pb._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, am->getShaderModule("mvp_mesh.vert")));
    pb._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, am->getShaderModule("basic.frag")));

    auto mvpMeshPipe = pb.build_pipeline(device, pipeline_rendering_create_info);
    pipes.push_back(mvpMeshPipe);

    //skybox pipeline
    VkPipelineLayoutCreateInfo skybox_pipeline_layout_info = vkinit::pipeline_layout_create_info();
    push_constant.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    skybox_pipeline_layout_info.pPushConstantRanges = &push_constant;
    skybox_pipeline_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout skybox_pipe_layout;
    VK_CHECK(vkCreatePipelineLayout(device, &skybox_pipeline_layout_info, nullptr, &skybox_pipe_layout));
    pb._pipelineLayout = skybox_pipe_layout;
    layouts.push_back(skybox_pipe_layout);

    pb._vertexInputInfo.vertexAttributeDescriptionCount = 0;
    pb._vertexInputInfo.vertexBindingDescriptionCount = 0;

    pb._shaderStages.clear();

    pb._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_VERTEX_BIT, am->getShaderModule("fullscreen_tri.vert")));

    pb._shaderStages.push_back(vkinit::pipeline_shader_stage_create_info(VK_SHADER_STAGE_FRAGMENT_BIT, am->getShaderModule("skybox.frag")));
    pb._pipelineLayout = skybox_pipe_layout;
    auto skybox_pipeline = pb.build_pipeline(device, pipeline_rendering_create_info);
    pipes.push_back(skybox_pipeline);

    Material skybox = { .layout = skybox_pipe_layout, .pipe = skybox_pipeline };
    materials["skybox"] = skybox;

    Material meshMat = { .layout = mesh_pipe_layout, .pipe = mvpMeshPipe };
    materials["Mesh"] = meshMat;

    //After all pipelines have been created, do the cleanup
    for (auto p : pipes) {
        cleanup_queue.push_function([=]() {
            vkDestroyPipeline(device, p, nullptr);
            });
    }

    for (auto pl : layouts) {
        cleanup_queue.push_function([=]() {
            vkDestroyPipelineLayout(device, pl, nullptr);
            });
    }

}

void RasterEngine::init_materials()
{
}

void RasterEngine::init_output()
{
    auto image_info = vkinit::image_create_info();
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_info.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    image_info.extent.depth = 1;

    image_info.extent.height = windowExtent.height;
    image_info.extent.width = windowExtent.width;
    image_info.format = outputFormat;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VmaAllocationCreateInfo malloc_info = {};
    malloc_info.memoryTypeBits = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    VK_CHECK(vmaCreateImage(allocator, &image_info, &malloc_info, &outputImage, &outputImageAllocation, nullptr));
    cleanup_queue.push_function([=] {
        vmaDestroyImage(allocator, outputImage, outputImageAllocation);
        });

    auto image_view_info = vkinit::image_view_create_info(image_info, outputImage);
    VK_CHECK(vkCreateImageView(device, &image_view_info, nullptr, &outputImageView));
    cleanup_queue.push_function([=] {
        vkDestroyImageView(device, outputImageView, nullptr);
        });


    const VkDebugUtilsObjectNameInfoEXT imageNameInfo =
    {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = NULL,
        .objectType = VK_OBJECT_TYPE_IMAGE,
        .objectHandle = (uint64_t)outputImage,
        .pObjectName = "Raster Output Image",
    };
    pfnSetDebugUtilsObjectNameEXT(device, &imageNameInfo);

}

void RasterEngine::init_draw_barriers()
{
    pre_draw_image_memory_barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
    .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .image = outputImage,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1,
    }
    };

    post_draw_image_memory_barrier = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
    .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
    .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
    .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    .image = outputImage,
    .subresourceRange = {
      .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
      .baseMipLevel = 0,
      .levelCount = 1,
      .baseArrayLayer = 0,
      .layerCount = 1
    }
    };
}

RasterEngine::Material RasterEngine::get_material(std::string mId)
{
    
    if (!materials.contains(mId))
    {
        //std::cout << "Raster Engine does not have requested material: " << mId << std::endl;
        return materials["Mesh"];
    }
    return materials[mId];
}
