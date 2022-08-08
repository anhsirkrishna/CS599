
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <math.h>

#include "vkapp.h"

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#include "app.h"
#include "shaders/shared_structs.h"


void VkApp::createRtBuffers()
{
    m_rtColCurrBuffer = createBufferImage(windowSize);
    transitionImageLayout(m_rtColCurrBuffer.image, vk::Format::eR32G32B32A32Sfloat,
                          vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, 1);
    // @@ Destroy whatever buffers were created.  Note: There will
    // ultimately be far more than just this one.
}

// Initialize ray tracing
void VkApp::initRayTracing()
{
    m_pcRay.rr = 0.8;
    m_pcRay.emissionFactor = 1.2;
    // Requesting ray tracing properties
    //VkPhysicalDeviceProperties2 prop2{VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    //vk::PhysicalDeviceProperties2 prop2;
    //VkPhysicalDeviceRayTracingPipelinePropertiesKHR rtProps
    //    {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR};
    //vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtProps;
    //prop2.pNext = &rtProps;
    //vkGetPhysicalDeviceProperties2(m_physicalDevice, &prop2);
    //m_physicalDevice.getProperties2(&prop2);

    auto allRtProps = m_physicalDevice.getProperties2<vk::PhysicalDeviceProperties2, vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rtPLProps = allRtProps.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
    handleSize      = rtPLProps.shaderGroupHandleSize;
    handleAlignment = rtPLProps.shaderGroupHandleAlignment;
    baseAlignment   = rtPLProps.shaderGroupBaseAlignment;
    
    m_rtBuilder.setup(this, m_device, m_graphicsQueueIndex);

    // @@ Call  m_rtBuilder.destroy() after the acceleration building is done.
}

//-------------------------------------------------------------------------------------------------
// This descriptor set holds the Acceleration structure and the output image
//
void VkApp::createRtDescriptorSet()
{   
    m_rtDesc.setBindings(m_device, {
        {0, vk::DescriptorType::eAccelerationStructureKHR, 1,
         vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR},
        {1, vk::DescriptorType::eStorageImage, 1,
         vk::ShaderStageFlagBits::eRaygenKHR}
        });

    m_rtDesc.write(m_device, 0, m_rtBuilder.getAccelerationStructure());
    m_rtDesc.write(m_device, 1, m_rtColCurrBuffer.Descriptor());
}

// Pipeline for the ray tracer: all shaders, raygen, chit, miss
//
void VkApp::createRtPipeline()
{
    ////////////////////////////////////////////////////////////////////////////////////////////
    // stages: Array of shaders: 1 raygen, 1 miss, 1 hit (later: an additional hit/miss pair.)

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Group the shaders.  Raygen and miss shaders get their own
    // groups. Hit shaders can group with any-hit and intersection
    // shaders -- of which we have none -- so the hit shader(s) get
    // their own group also.
    std::vector<vk::PipelineShaderStageCreateInfo> stages{};
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups{};

    vk::PipelineShaderStageCreateInfo stage;
    stage.setPName("main");  // All the same entry point

    vk::RayTracingShaderGroupCreateInfoKHR group;
    group.anyHitShader       = VK_SHADER_UNUSED_KHR;
    group.closestHitShader   = VK_SHADER_UNUSED_KHR;
    group.generalShader      = VK_SHADER_UNUSED_KHR;
    group.intersectionShader = VK_SHADER_UNUSED_KHR;

    // Raygen shader stage and group appended to stages and groups lists
    stage.module = createShaderModule(loadFile("spv/raytrace.rgen.spv"));
    //stage.stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR;
    stage.setStage(vk::ShaderStageFlagBits::eRaygenKHR);
    stages.push_back(stage);
    
    //group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    group.generalShader = stages.size()-1;    // Index of raygen shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;
    
    // Miss shader stage and group appended to stages and groups lists
    stage.module = createShaderModule(loadFile("spv/raytrace.rmiss.spv"));
    //stage.stage = VK_SHADER_STAGE_MISS_BIT_KHR;
    stage.setStage(vk::ShaderStageFlagBits::eMissKHR);
    stages.push_back(stage);
    
    //group.type          = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
    group.setType(vk::RayTracingShaderGroupTypeKHR::eGeneral);
    group.generalShader = stages.size()-1;    // Index of miss shader
    groups.push_back(group);
    group.generalShader    = VK_SHADER_UNUSED_KHR;

    // Closest hit shader stage and group appended to stages and groups lists
    stage.module = createShaderModule(loadFile("spv/raytrace.rchit.spv"));
    //stage.stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
    stage.setStage(vk::ShaderStageFlagBits::eClosestHitKHR);
    stages.push_back(stage);

    //group.type             = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    group.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
    group.closestHitShader = stages.size()-1;   // Index of hit shader
    groups.push_back(group);


    ////////////////////////////////////////////////////////////////////////////////////////////
    // Create the ray tracing pipeline layout.
    // Push constant: we want to be able to update constants used by the shaders
    vk::PushConstantRange pushConstant;
    pushConstant.setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR);
    pushConstant.setOffset(0);
    pushConstant.setSize(sizeof(PushConstantRay));

    vk::PipelineLayoutCreateInfo pipelineLayoutCreateInfo;
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges    = &pushConstant;

    // Descriptor sets: one specific to ray tracing, and one shared with the rasterization pipeline
    std::vector<vk::DescriptorSetLayout> rtDescSetLayouts =
        {m_rtDesc.descSetLayout, m_scDesc.descSetLayout};
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(rtDescSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = rtDescSetLayouts.data();

    //vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);
    m_device.createPipelineLayout(&pipelineLayoutCreateInfo, nullptr, &m_rtPipelineLayout);

    ////////////////////////////////////////////////////////////////////////////////////////////
    // Create the ray tracing pipeline.
    // Assemble the shader stages and recursion depth info into the ray tracing pipeline
    vk::RayTracingPipelineCreateInfoKHR rayPipelineInfo;
    rayPipelineInfo.stageCount = static_cast<uint32_t>(stages.size());  // Stages are shaders
    rayPipelineInfo.pStages    = stages.data();

    rayPipelineInfo.groupCount = static_cast<uint32_t>(groups.size());
    rayPipelineInfo.pGroups    = groups.data();

    rayPipelineInfo.maxPipelineRayRecursionDepth = 10;  // Ray depth
    rayPipelineInfo.layout                       = m_rtPipelineLayout;

    //vkCreateRayTracingPipelinesKHR(m_device, {}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);
    m_device.createRayTracingPipelinesKHR({}, {}, 1, &rayPipelineInfo, nullptr, &m_rtPipeline);
    for (auto& s : stages)
        //vkDestroyShaderModule(m_device, s.module, nullptr);
        m_device.destroyShaderModule(s.module, nullptr);

    // @@ Destroy pipeline and its layout with
    //   vkDestroyPipelineLayout(m_device, m_rtPipelineLayout, nullptr);
    //   vkDestroyPipeline(m_device, m_rtPipeline, nullptr);
}

//--------------------------------------------------------------------------------------------------
// The Shader Binding Table (SBT)
// - getting all shader handles and write them in a SBT buffer
//
template <class integral>
constexpr integral align_up(integral x, size_t a) noexcept
{
    return integral((x + (integral(a) - 1)) & ~integral(a - 1));
}

void VkApp::createRtShaderBindingTable()
{
    uint32_t missCount{1};
    uint32_t hitCount{1};
    auto     handleCount = 1 + missCount + hitCount;

    // The SBT (buffer) needs to have starting group to be aligned
    // and handles in the group to be aligned.
    uint32_t handleSizeAligned = align_up(handleSize, handleAlignment);  // handleAlignment==32

    m_rgenRegion.stride = align_up(handleSizeAligned, baseAlignment); //baseAlignment==64
    m_rgenRegion.size = m_rgenRegion.stride;  // The size member must be equal to its stride member
    
    m_missRegion.stride = handleSizeAligned;
    m_missRegion.size   = align_up(missCount * handleSizeAligned, baseAlignment);
    
    m_hitRegion.stride  = handleSizeAligned;
    m_hitRegion.size    = align_up(hitCount * handleSizeAligned, baseAlignment);

    printf("Shader binding table:\n");
    printf("  alignments:\n");
    printf("    handleAlignment: %d\n", handleAlignment);
    printf("    baseAlignment:   %d\n", baseAlignment);
    printf("  counts:\n");
    printf("    miss:   %d\n", missCount);
    printf("    hit:    %d\n", hitCount);
    printf("    handle: %d = 1+missCount+hitCount\n", handleCount);
    printf("  regions stride:size:\n");
    printf("    rgen %2ld:%2ld\n", m_rgenRegion.stride, m_rgenRegion.size);
    printf("    miss %2ld:%2ld\n", m_missRegion.stride, m_missRegion.size);
    printf("    hit  %2ld:%2ld\n", m_hitRegion.stride,  m_hitRegion.size);
    printf("    call %2ld:%2ld\n", m_callRegion.stride, m_callRegion.size);

    // Get the shader group handles.  This is a byte array retrieved
    // from the pipeline.
    uint32_t             dataSize = handleCount * handleSize;
    std::vector<uint8_t> handles(dataSize);
    //auto result = vkGetRayTracingShaderGroupHandlesKHR(m_device, m_rtPipeline,
    //                                                   0, handleCount, dataSize, handles.data());
    auto result = m_device.getRayTracingShaderGroupHandlesKHR(m_rtPipeline,
        0, handleCount, dataSize, handles.data());
    assert(result == vk::Result::eSuccess);

    // Allocate a buffer for storing the SBT, and a staging buffer for transferring data to it.
    vk::DeviceSize sbtSize = m_rgenRegion.size + m_missRegion.size
        + m_hitRegion.size + m_callRegion.size;
    
    BufferWrap staging = createBufferWrap(sbtSize, vk::BufferUsageFlagBits::eTransferSrc,
                                      vk::MemoryPropertyFlagBits::eHostVisible
                                      | vk::MemoryPropertyFlagBits::eHostCoherent);
    m_shaderBindingTableBW = createBufferWrap(sbtSize,
                                    vk::BufferUsageFlagBits::eTransferDst
                                  | vk::BufferUsageFlagBits::eShaderDeviceAddress
                                  | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
                                    vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Find the SBT addresses of each group
    vk::BufferDeviceAddressInfo info;
    info.buffer                    = m_shaderBindingTableBW.buffer;
    vk::DeviceAddress sbtAddress = m_device.getBufferAddress(&info);
    
    m_rgenRegion.deviceAddress = sbtAddress;
    m_missRegion.deviceAddress = sbtAddress + m_rgenRegion.size;
    m_hitRegion.deviceAddress  = sbtAddress + m_rgenRegion.size + m_missRegion.size;

    // Helper to retrieve the handle data
    auto getHandle = [&](int i) { return handles.data() + i * handleSize; };

    // Map the SBT buffer and write in the handles.
    uint8_t* mappedMemAddress;
    //vkMapMemory(m_device, staging.memory, 0, sbtSize, 0, (void**)&mappedMemAddress);
    m_device.mapMemory(staging.memory, 0, sbtSize, vk::MemoryMapFlags(), (void**)&mappedMemAddress);
    uint8_t offset = 0;

    // Raygen
    uint32_t handleIdx{0};
    memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);

    // Miss
    offset = m_rgenRegion.size;
    for(uint32_t c = 0; c < missCount; c++) {
        memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);
        offset += m_missRegion.stride; }

    // Hit
    offset = m_rgenRegion.size + m_missRegion.size;
    for(uint32_t c = 0; c < hitCount; c++) {
        memcpy(mappedMemAddress+offset, getHandle(handleIdx++), handleSize);
        offset += m_hitRegion.stride; }

    //vkUnmapMemory(m_device, staging.memory);
    m_device.unmapMemory(staging.memory);
    
    copyBuffer(staging.buffer, m_shaderBindingTableBW.buffer, sbtSize);

    staging.destroy(m_device);

    // @@ destroy acceleration structure with m_shaderBindingTableBW.destroy(m_device);
}

void VkApp::raytrace()
{
    // The (temporary) push constants for the ray tracing pipeline.
    m_pcRay.tempLightPos = vec4(0.5f, 2.5f, 3.0f, 0.0);
    m_pcRay.tempLightInt = vec4(2.5, 2.5, 2.5, 0.0);
    m_pcRay.tempAmbient = vec4(0.2);

    m_pcRay.depth = 1;
    while (float(rand()) / RAND_MAX < m_pcRay.rr)   m_pcRay.depth++;

    m_pcRay.frameSeed = rand() % 32768;

    m_pcRay.specular = specularOn;

    // Bind the ray tracing pipeline
    //vkCmdBindPipeline(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, m_rtPipeline);
    m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, m_rtPipeline);

    // Bind the descriptor sets (the ray tracing specific one, and the
    // full model descriptor)
    std::vector<vk::DescriptorSet> descSets{m_rtDesc.descSet, m_scDesc.descSet};
    //vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
    //                       m_rtPipelineLayout, 0, (uint32_t)descSets.size(),
    //                        descSets.data(), 0, nullptr);
    m_commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR,
                                        m_rtPipelineLayout, 0, (uint32_t)descSets.size(),
                                        descSets.data(), 0, nullptr);

    // Push the push constants
    /*
    vkCmdPushConstants(m_commandBuffer, m_rtPipelineLayout,
                       VK_SHADER_STAGE_RAYGEN_BIT_KHR
                       | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR
                       | VK_SHADER_STAGE_MISS_BIT_KHR,
                       0, sizeof(PushConstantRay), &m_pcRay);*/
    m_commandBuffer.pushConstants(m_rtPipelineLayout, 
        vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eMissKHR,
        0, sizeof(PushConstantRay), &m_pcRay);

    //vkCmdTraceRaysKHR(m_commandBuffer, &m_rgenRegion, &m_missRegion, &m_hitRegion,
    //                 &m_callRegion, windowSize.width, windowSize.height, 1);
    m_commandBuffer.traceRaysKHR(&m_rgenRegion, &m_missRegion, &m_hitRegion,
        &m_callRegion, windowSize.width, windowSize.height, 1);

    // Copy the ray tracer output image to the scanline output image
    // -- because we already have the operations needed to display
    // that image on the screen.
    vk::ImageCopy imageCopyRegion;
    imageCopyRegion.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopyRegion.srcSubresource.layerCount = 1;
    imageCopyRegion.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
    imageCopyRegion.dstSubresource.layerCount = 1;
    imageCopyRegion.extent.width              = windowSize.width;
    imageCopyRegion.extent.height             = windowSize.height;
    imageCopyRegion.extent.depth              = 1;

    imageLayoutBarrier(m_commandBuffer, m_rtColCurrBuffer.image, 
        vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferSrcOptimal);
    imageLayoutBarrier(m_commandBuffer, m_scImageBuffer.image,
        vk::ImageLayout::eGeneral, vk::ImageLayout::eTransferDstOptimal);
    /*
    vkCmdCopyImage(m_commandBuffer,
                   m_rtColCurrBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   m_scImageBuffer.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &imageCopyRegion);*/
    m_commandBuffer.copyImage(m_rtColCurrBuffer.image, vk::ImageLayout::eTransferSrcOptimal,
        m_scImageBuffer.image, vk::ImageLayout::eTransferDstOptimal,
        1, &imageCopyRegion);

    imageLayoutBarrier(m_commandBuffer, m_scImageBuffer.image,
        vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eGeneral);
    imageLayoutBarrier(m_commandBuffer, m_rtColCurrBuffer.image,
        vk::ImageLayout::eTransferSrcOptimal, vk::ImageLayout::eGeneral);
}

