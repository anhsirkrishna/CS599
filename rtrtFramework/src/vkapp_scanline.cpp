
#include <iostream>
#include <fstream>
#include <string>
#include <cstring>              // for memcpy
#include <vector>
#include <array>
#include <math.h>

#include "vkapp.h"

#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
using namespace glm;

#define STBI_FAILURE_USERMSG
#include "stb_image.h"

#include "app.h"
#include "shaders/shared_structs.h"

/*
VkAccessFlags accessFlagsForImageLayout(VkImageLayout layout)
{
    switch(layout)
        {
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_ACCESS_HOST_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        default:
            return VkAccessFlags();
            return vk::AccessFlags();
        }
}*/

vk::AccessFlags accessFlagsForImageLayout(vk::ImageLayout layout)
{
    switch (layout)
    {
    case vk::ImageLayout::ePreinitialized:
        return vk::AccessFlagBits::eHostWrite;
    case vk::ImageLayout::eTransferDstOptimal:
        return vk::AccessFlagBits::eTransferWrite;
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::AccessFlagBits::eTransferRead;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::AccessFlagBits::eColorAttachmentWrite;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::AccessFlagBits::eShaderRead;
    default:
        return vk::AccessFlags();
    }
}

/*
VkPipelineStageFlags pipelineStageForLayout(VkImageLayout layout)
{
    switch(layout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // Allow queue other than graphic
            // return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;  // Allow queue other than graphic
            // return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_HOST_BIT;
        case VK_IMAGE_LAYOUT_UNDEFINED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        default:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        }
}*/

vk::PipelineStageFlags pipelineStageForLayout(vk::ImageLayout layout)
{
    switch (layout)
    {
    case vk::ImageLayout::eTransferDstOptimal:
    case vk::ImageLayout::eTransferSrcOptimal:
        return vk::PipelineStageFlagBits::eTransfer;
    case vk::ImageLayout::eColorAttachmentOptimal:
        return vk::PipelineStageFlagBits::eColorAttachmentOutput;
    case vk::ImageLayout::eDepthStencilAttachmentOptimal:
        return vk::PipelineStageFlagBits::eAllCommands;  // Allow queue other than graphic
        // return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    case vk::ImageLayout::eShaderReadOnlyOptimal:
        return vk::PipelineStageFlagBits::eAllCommands;  // Allow queue other than graphic
        // return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    case vk::ImageLayout::ePreinitialized:
        return vk::PipelineStageFlagBits::eHost;
    case vk::ImageLayout::eUndefined:
        return vk::PipelineStageFlagBits::eTopOfPipe;
    default:
        return vk::PipelineStageFlagBits::eBottomOfPipe;
    }
}

/*
void imageLayoutBarrier(VkCommandBuffer cmdbuffer,
                        VkImage image,
                        VkImageLayout oldImageLayout,
                        VkImageLayout newImageLayout,
                        VkImageAspectFlags aspectMask=VK_IMAGE_ASPECT_COLOR_BIT)
{
    VkImageSubresourceRange subresourceRange;
    subresourceRange.aspectMask     = aspectMask;
    subresourceRange.levelCount     = VK_REMAINING_MIP_LEVELS;
    subresourceRange.layerCount     = VK_REMAINING_ARRAY_LAYERS;
    subresourceRange.baseMipLevel   = 0;
    subresourceRange.baseArrayLayer = 0;
  
    VkImageMemoryBarrier imageMemoryBarrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    imageMemoryBarrier.oldLayout        = oldImageLayout;
    imageMemoryBarrier.newLayout        = newImageLayout;
    imageMemoryBarrier.image            = image;
    imageMemoryBarrier.subresourceRange = subresourceRange;
    imageMemoryBarrier.srcAccessMask    = accessFlagsForImageLayout(oldImageLayout);
    imageMemoryBarrier.dstAccessMask    = accessFlagsForImageLayout(newImageLayout);
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    VkPipelineStageFlags srcStageMask      = pipelineStageForLayout(oldImageLayout);
    VkPipelineStageFlags destStageMask     = pipelineStageForLayout(newImageLayout);
  
    vkCmdPipelineBarrier(cmdbuffer, srcStageMask, destStageMask, 0,
                         0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}*/

void imageLayoutBarrier(vk::CommandBuffer cmdbuffer,
                        vk::Image image,
                        vk::ImageLayout oldImageLayout,
                        vk::ImageLayout newImageLayout,
                        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor)
{
    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setAspectMask(aspectMask);
    subresourceRange.setLevelCount(VK_REMAINING_MIP_LEVELS);
    subresourceRange.setLayerCount(VK_REMAINING_ARRAY_LAYERS);
    subresourceRange.setBaseMipLevel(0);
    subresourceRange.setBaseArrayLayer(0);

    vk::ImageMemoryBarrier imageMemoryBarrier;

    imageMemoryBarrier.setOldLayout(oldImageLayout);
    imageMemoryBarrier.setNewLayout(newImageLayout);
    imageMemoryBarrier.setImage(image);
    imageMemoryBarrier.setSubresourceRange(subresourceRange);
    imageMemoryBarrier.setSrcAccessMask(accessFlagsForImageLayout(oldImageLayout));
    imageMemoryBarrier.setDstAccessMask(accessFlagsForImageLayout(newImageLayout));
    imageMemoryBarrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    imageMemoryBarrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    vk::PipelineStageFlags srcStageMask = pipelineStageForLayout(oldImageLayout);
    vk::PipelineStageFlags destStageMask = pipelineStageForLayout(newImageLayout);

    cmdbuffer.pipelineBarrier(srcStageMask, destStageMask, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &imageMemoryBarrier);
}

ImageWrap VkApp::createTextureImage(std::string fileName)
{
    //VkImage& textureImage, VkDeviceMemory& textureImageMemory
    int texWidth, texHeight, texChannels;

    stbi_set_flip_vertically_on_load(true);
    stbi_uc* pixels = stbi_load(fileName.c_str(), &texWidth, &texHeight, &texChannels,
                                STBI_rgb_alpha);
    vk::DeviceSize imageSize = texWidth * texHeight * 4;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    BufferWrap staging = createBufferWrap(imageSize, vk::BufferUsageFlagBits::eTransferSrc,
                                        vk::MemoryPropertyFlagBits::eHostVisible
                                        | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* data;
    m_device.mapMemory(staging.memory, 0, imageSize, vk::MemoryMapFlags(), &data);
    memcpy(data, pixels, static_cast<size_t>(imageSize));
    m_device.unmapMemory(staging.memory);

    stbi_image_free(pixels);

    uint mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
    /*
    ImageWrap myImage = createImageWrap(texWidth, texHeight, VK_FORMAT_R8G8B8A8_UNORM,
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                  | VK_IMAGE_USAGE_SAMPLED_BIT
                                  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  mipLevels);*/

    ImageWrap myImage = createImageWrap(texWidth, texHeight, vk::Format::eB8G8R8A8Unorm,
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eSampled,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        mipLevels
    );

    //transitionImageLayout(myImage.image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED,
    //                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
    transitionImageLayout(myImage.image, vk::Format::eB8G8R8A8Unorm, vk::ImageLayout::eUndefined, 
                          vk::ImageLayout::eTransferDstOptimal, mipLevels);
    copyBufferToImage(staging.buffer, myImage.image, static_cast<uint32_t>(texWidth),
                      static_cast<uint32_t>(texHeight));

    staging.destroy(m_device);

    generateMipmaps(myImage.image, VK_FORMAT_R8G8B8A8_UNORM, texWidth, texHeight, mipLevels);
    
    vk::ImageAspectFlags aspect;
    myImage.imageView = createImageView(myImage.image, vk::Format::eB8G8R8A8Unorm, vk::ImageAspectFlagBits::eColor);
    myImage.sampler = createTextureSampler();
    myImage.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    return myImage;
}

void VkApp::generateMipmaps(VkImage image, VkFormat imageFormat,
                            int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(m_physicalDevice, imageFormat, &formatProperties);
    
    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer = createTempCmdBuffer();

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
                       image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blit,
                       VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                             0, nullptr,
                             0, nullptr,
                             1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                         0, nullptr,
                         0, nullptr,
                         1, &barrier);

    submitTempCmdBuffer(commandBuffer);
}

void VkApp::generateMipmaps(vk::Image image, vk::Format imageFormat,
                            int32_t texWidth, int32_t texHeight, uint32_t mipLevels)
{
    // Check if image format supports linear blitting
    vk::FormatProperties formatProperties = m_physicalDevice.getFormatProperties(imageFormat);
    if (!(formatProperties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    vk::CommandBuffer commandBuffer = createTempCppCmdBuffer();


    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);

    vk::ImageSubresourceRange subresourceRange;
    subresourceRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    subresourceRange.setLevelCount(1);
    subresourceRange.setLayerCount(1);
    subresourceRange.setBaseArrayLayer(0);

    barrier.setSubresourceRange(subresourceRange);

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        subresourceRange.setBaseMipLevel(i - 1);
        barrier.setSubresourceRange(subresourceRange);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(),
            0, nullptr, 0, nullptr, 1, &barrier);

        vk::ImageBlit blit;
        
        std::array<vk::Offset3D, 2> srcOffsets;
        srcOffsets[0] = vk::Offset3D{};
        srcOffsets[1] = vk::Offset3D{ mipWidth, mipHeight, 1 };
        blit.setSrcOffsets(srcOffsets);
        vk::ImageSubresourceLayers srcsubresourceLayers;
        srcsubresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);
        srcsubresourceLayers.setMipLevel(i - 1);
        srcsubresourceLayers.setBaseArrayLayer(0);
        srcsubresourceLayers.setLayerCount(1);
        blit.setSrcSubresource(srcsubresourceLayers);
        
        std::array<vk::Offset3D, 2> dstOffsets;
        dstOffsets[0] = vk::Offset3D{ 0, 0, 0 };
        dstOffsets[1] = vk::Offset3D{ mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.setDstOffsets(dstOffsets);
        vk::ImageSubresourceLayers dstsubresourceLayers;
        dstsubresourceLayers.setAspectMask(vk::ImageAspectFlagBits::eColor);
        dstsubresourceLayers.setMipLevel(i);
        dstsubresourceLayers.setBaseArrayLayer(0);
        dstsubresourceLayers.setLayerCount(1);
        blit.setDstSubresource(dstsubresourceLayers);

        commandBuffer.blitImage(image, vk::ImageLayout::eTransferSrcOptimal, 
                                image, vk::ImageLayout::eTransferDstOptimal, 
                                1, &blit, vk::Filter::eLinear);

        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal);
        barrier.setNewLayout(vk::ImageLayout::eTransferDstOptimal);
        barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferRead);
        barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

        commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(),
            0, nullptr, 0, nullptr, 1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    subresourceRange.setBaseMipLevel(mipLevels - 1);
    barrier.setSubresourceRange(subresourceRange);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
    barrier.setNewLayout(vk::ImageLayout::eTransferSrcOptimal);
    barrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    barrier.setDstAccessMask(vk::AccessFlagBits::eTransferRead);

    commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, vk::DependencyFlags(),
        0, nullptr, 0, nullptr, 1, &barrier);

    submitTemptCppCmdBuffer(commandBuffer);
}

/*
BufferWrap VkApp::createStagedBufferWrap(const vk::CommandBuffer& cmdBuf,
                                         const vk::DeviceSize&    size,
                                         const void*            data,
                                         vk::BufferUsageFlags     usage)
{
    BufferWrap staging = createBufferWrap(size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                      | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void* dest;
    vkMapMemory(m_device, staging.memory, 0, size, 0, &dest);
    memcpy(dest, data, size);
    vkUnmapMemory(m_device, staging.memory);

    
    BufferWrap bw = createBufferWrap(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
                                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    copyBuffer(staging.buffer, bw.buffer, size);

    VkDevice temp_device(m_device);
    staging.destroy(temp_device);
    
    return bw;
}*/

BufferWrap VkApp::createStagedBufferWrap(const vk::CommandBuffer& cmdBuf,
                                            const vk::DeviceSize& size,
                                            const void* data,
                                            vk::BufferUsageFlags usage)
{
    BufferWrap staging = createBufferWrap(size, vk::BufferUsageFlagBits::eTransferSrc,
        vk::MemoryPropertyFlagBits::eHostVisible
        | vk::MemoryPropertyFlagBits::eHostCoherent);

    void* dest;
    m_device.mapMemory(staging.memory, 0, size, vk::MemoryMapFlags(), &dest);
    memcpy(dest, data, size);
    m_device.unmapMemory(staging.memory);


    BufferWrap bw = createBufferWrap(size, vk::BufferUsageFlagBits::eTransferDst | usage,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    copyBuffer(staging.buffer, bw.buffer, size);

    staging.destroy(m_device);

    return bw;
}

/*
BufferWrap VkApp::createBufferWrap(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                      vk::MemoryPropertyFlags properties)
{
    BufferWrap result;
    
    VkBufferCreateInfo bufferInfo{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(m_device, &bufferInfo, nullptr, &result.buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_device, result.buffer, &memRequirements);

    VkMemoryAllocateFlagsInfo memFlags = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO, nullptr,
        VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT, 0};

    VkMemoryAllocateInfo allocInfo{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocInfo.pNext = &memFlags;
    allocInfo.allocationSize = memRequirements.size;
    vk::MemoryPropertyFlags temp_properties(properties);
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, temp_properties);

    if (vkAllocateMemory(m_device, &allocInfo, nullptr, &result.memory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }
        
    vkBindBufferMemory(m_device, result.buffer, result.memory, 0);

    return result;
}*/

BufferWrap VkApp::createBufferWrap(vk::DeviceSize size, vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties)
{
    BufferWrap result;

    vk::BufferCreateInfo bufferInfo;
    bufferInfo.setSize(size);
    bufferInfo.setUsage(usage);
    bufferInfo.setSharingMode(vk::SharingMode::eExclusive);

    m_device.createBuffer(&bufferInfo, nullptr, &result.buffer);

    vk::MemoryRequirements memRequirements;
    m_device.getBufferMemoryRequirements(result.buffer, &memRequirements);

    vk::MemoryAllocateFlagsInfo memFlags;
    memFlags.setFlags(vk::MemoryAllocateFlagBits::eDeviceAddress);
    memFlags.setDeviceMask(0);

    vk::MemoryAllocateInfo allocInfo;

    allocInfo.setPNext(&memFlags);
    allocInfo.setAllocationSize(memRequirements.size);
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (m_device.allocateMemory(&allocInfo, nullptr, &result.memory) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    m_device.bindBufferMemory(result.buffer, result.memory, 0);

    return result;
}

void VkApp::copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size)
{
    vk::CommandBuffer commandBuffer =  createTempCppCmdBuffer();

    vk::BufferCopy copyRegion;
    copyRegion.setSize(size);

    commandBuffer.copyBuffer(srcBuffer, dstBuffer, 1, &copyRegion);

    submitTemptCppCmdBuffer(commandBuffer);
}


void VkApp::copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = createTempCmdBuffer();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
    
    submitTempCmdBuffer(commandBuffer);
}

/*
void VkApp::transitionImageLayout(VkImage image,
                                        VkFormat format,
                                        VkImageLayout oldLayout,
                                        VkImageLayout newLayout,
                                        uint32_t mipLevels)
{
    VkCommandBuffer commandBuffer = createTempCmdBuffer();

    VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = mipLevels;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        
    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    vkCmdPipelineBarrier(commandBuffer, sourceStage, destinationStage, 0,
                         0, nullptr,    0, nullptr,    1, &barrier);
    
    submitTempCmdBuffer(commandBuffer);
}*/

void VkApp::transitionImageLayout(vk::Image image,
                                  vk::Format format,
                                  vk::ImageLayout oldLayout,
                                  vk::ImageLayout newLayout,
                                  uint32_t mipLevels)
{
    vk::CommandBuffer commandBuffer = createTempCppCmdBuffer();

    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(oldLayout);
    barrier.setNewLayout(newLayout);
    barrier.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED);
    barrier.setImage(image);

    vk::ImageSubresourceRange subResRange;
    subResRange.setAspectMask(vk::ImageAspectFlagBits::eColor);
    subResRange.setBaseMipLevel(0);
    subResRange.setLevelCount(mipLevels);
    subResRange.setBaseArrayLayer(0);
    subResRange.setLayerCount(1);
    barrier.setSubresourceRange(subResRange);
    barrier.setSrcAccessMask(vk::AccessFlagBits::eNone);
    barrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

    vk::PipelineStageFlags sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
    vk::PipelineStageFlags destinationStage = vk::PipelineStageFlagBits::eTransfer;
    
    commandBuffer.pipelineBarrier(sourceStage, destinationStage, vk::DependencyFlags(), 0, nullptr, 0, nullptr, 1, &barrier);

    submitTemptCppCmdBuffer(commandBuffer);
}

VkSampler VkApp::createTextureSampler()
{
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkSampler textureSampler;
    if (vkCreateSampler(m_device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
    return textureSampler;
}

void VkApp::createPostDescriptor()
{
    /*
    m_postDesc.setBindings(m_device, {
            {0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT}
        });
    m_postDesc.write(m_device, 0, m_scImageBuffer.Descriptor());
    */
    m_postDesc.setBindings(m_device, {
            {0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment}
        });
    m_postDesc.write(m_device, 0, m_scImageBuffer.Descriptor());

    //
    // @@ Destroy with m_postDesc.destroy(m_device);
}

void VkApp::createScBuffer()
{
    m_scImageBuffer = createBufferImage(windowSize);

    vk::CommandBuffer    cmdBuf = createTempCppCmdBuffer();
    imageLayoutBarrier(cmdBuf, m_scImageBuffer.image,
                        vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    //submitTempCmdBuffer(cmdBuf);
    submitTemptCppCmdBuffer(cmdBuf);

    // @@ Destroy with m_scImageBuffer.destroy(m_device);
}

ImageWrap VkApp::createBufferImage(vk::Extent2D& size)
{
    //uint mipLevels = std::floor(std::log2(std::max(texWidth, texHeight))) + 1;
    uint mipLevels = 1;
    /*
    ImageWrap myImage = createImageWrap(size.width, size.height, VK_FORMAT_R32G32B32A32_SFLOAT,
                                  VK_IMAGE_USAGE_TRANSFER_DST_BIT
                                  | VK_IMAGE_USAGE_SAMPLED_BIT
                                  | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                                  | VK_IMAGE_USAGE_STORAGE_BIT
                                  | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                  mipLevels);*/
    ImageWrap myImage = createImageWrap(size.width, size.height, vk::Format::eR32G32B32A32Sfloat,
        vk::ImageUsageFlagBits::eTransferDst |
        vk::ImageUsageFlagBits::eSampled |
        vk::ImageUsageFlagBits::eStorage |
        vk::ImageUsageFlagBits::eTransferSrc |
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        mipLevels
    );

    //myImage.imageView = createImageView(myImage.image, VK_FORMAT_R32G32B32A32_SFLOAT);
    vk::ImageAspectFlags aspect;
    myImage.imageView = createImageView(myImage.image, vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor);
    myImage.sampler = createTextureSampler();
    //myImage.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
    myImage.imageLayout = vk::ImageLayout::eGeneral;
    return myImage;
}

// The scanline renderpass outputs to m_scImageBuffer (as wrapped by m_scanlineFramebuffer)
/*
void VkApp::createScanlineRenderPass()
{
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = VK_FORMAT_R32G32B32A32_SFLOAT;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_GENERAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format =  VK_FORMAT_X8_D24_UNORM_PACK32;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
        | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
        | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    
    std::array<VkAttachmentDescription, 2> attachmentsDsc = {colorAttachment, depthAttachment};
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentsDsc.size());
    renderPassInfo.pAttachments = attachmentsDsc.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_scanlineRenderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create scanline render pass!");
    }

    std::vector<VkImageView> attachments = {m_scImageBuffer.imageView, m_depthImage.imageView};

    VkFramebufferCreateInfo info{VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    info.renderPass      = m_scanlineRenderPass;
    info.attachmentCount = attachments.size();
    info.pAttachments    = attachments.data();
    info.width           = windowSize.width;
    info.height          = windowSize.height;
    info.layers          = 1;
    vkCreateFramebuffer(m_device, &info, nullptr, &m_scanlineFramebuffer);

    // @@ Destroy with vkDestroyRenderPass(m_device, m_scanlineRenderPass, nullptr);
    // @@ Destroy with vkDestroyFramebuffer(m_device, m_scanlineFramebuffer, nullptr);
}*/

void VkApp::createScanlineRenderPass()
{
    vk::AttachmentDescription colorAttachment;
    colorAttachment.setFormat(vk::Format::eR32G32B32A32Sfloat);
    colorAttachment.setSamples(vk::SampleCountFlagBits::e1);
    colorAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    colorAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    colorAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    colorAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    colorAttachment.setInitialLayout(vk::ImageLayout::eGeneral);
    colorAttachment.setFinalLayout(vk::ImageLayout::eGeneral);

    vk::AttachmentDescription depthAttachment;
    depthAttachment.setFormat(vk::Format::eX8D24UnormPack32);
    depthAttachment.setSamples(vk::SampleCountFlagBits::e1);
    depthAttachment.setLoadOp(vk::AttachmentLoadOp::eClear);
    depthAttachment.setStoreOp(vk::AttachmentStoreOp::eStore);
    depthAttachment.setStencilLoadOp(vk::AttachmentLoadOp::eDontCare);
    depthAttachment.setStencilStoreOp(vk::AttachmentStoreOp::eDontCare);
    depthAttachment.setInitialLayout(vk::ImageLayout::eUndefined);
    depthAttachment.setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::AttachmentReference colorAttachmentRef;
    colorAttachmentRef.setAttachment(0);
    colorAttachmentRef.setLayout(vk::ImageLayout::eColorAttachmentOptimal);


    vk::AttachmentReference depthAttachmentRef;
    depthAttachmentRef.setAttachment(1);
    depthAttachmentRef.setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    
    vk::SubpassDescription subpass;
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpass.setColorAttachmentCount(1);
    subpass.setPColorAttachments(&colorAttachmentRef);
    subpass.setPDepthStencilAttachment(&depthAttachmentRef);

    vk::SubpassDependency dependency;
    dependency.setSrcSubpass(VK_SUBPASS_EXTERNAL);
    dependency.setDstSubpass(0);
    dependency.setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput | 
                               vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setSrcAccessMask(vk::AccessFlagBits::eNoneKHR);
    dependency.setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput |
                               vk::PipelineStageFlagBits::eEarlyFragmentTests);
    dependency.setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite | 
                                vk::AccessFlagBits::eDepthStencilAttachmentWrite);

    std::array<vk::AttachmentDescription, 2> attachmentsDsc = { colorAttachment, depthAttachment };
    
    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.setAttachmentCount(static_cast<uint32_t>(attachmentsDsc.size()));
    renderPassInfo.setPAttachments(attachmentsDsc.data());
    renderPassInfo.setSubpassCount(1);
    renderPassInfo.setPSubpasses(&subpass);
    renderPassInfo.setDependencyCount(1);
    renderPassInfo.setPDependencies(&dependency);

    if (m_device.createRenderPass(&renderPassInfo, nullptr, &m_scanlineRenderPass) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create scanline render pass!");
    }

    std::vector<vk::ImageView> attachments = { m_scImageBuffer.imageView, m_depthImage.imageView };

    vk::FramebufferCreateInfo info;
    info.setRenderPass(m_scanlineRenderPass);
    info.setAttachmentCount(attachments.size());
    info.setPAttachments(attachments.data());
    info.setWidth(windowSize.width);
    info.setHeight(windowSize.height);
    info.setLayers(1);

    if (m_device.createFramebuffer(&info, nullptr, &m_scanlineFramebuffer) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create scanline render pass!");
    }

    // @@ Destroy with vkDestroyRenderPass(m_device, m_scanlineRenderPass, nullptr);
    // @@ Destroy with vkDestroyFramebuffer(m_device, m_scanlineFramebuffer, nullptr);
}

void VkApp::createScDescriptorSet()
{
    auto nbTxt = static_cast<uint32_t>(m_objText.size());

    /*
    m_scDesc.setBindings(m_device, {
            {ScBindings::eMatrices, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_RAYGEN_BIT_KHR},
            {ScBindings::eObjDescs, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
                | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR},
            {ScBindings::eTextures, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, nbTxt,
                VK_SHADER_STAGE_FRAGMENT_BIT | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR}
        });*/

    m_scDesc.setBindings(m_device, {
            {ScBindings::eMatrices, vk::DescriptorType::eUniformBuffer, 1,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eRaygenKHR},
            {ScBindings::eObjDescs, vk::DescriptorType::eStorageBuffer, 1,
                vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment
                | vk::ShaderStageFlagBits::eClosestHitKHR},
            {ScBindings::eTextures, vk::DescriptorType::eCombinedImageSampler, nbTxt,
                vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eClosestHitKHR}
        });
              
    m_scDesc.write(m_device, ScBindings::eMatrices, m_matrixBW.buffer);
    m_scDesc.write(m_device, ScBindings::eObjDescs, m_objDescriptionBW.buffer);
    m_scDesc.write(m_device, ScBindings::eTextures, m_objText);    

    // @@ Destroy with m_scDesc.destroy(m_device);
}
/*
void VkApp::createScPipeline()
{
    VkPushConstantRange pushConstantRanges = {
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstantRaster)};

    // Creating the Pipeline Layout
    VkPipelineLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO};
    createInfo.setLayoutCount         = 1;
    VkDescriptorSetLayout tempLayouts(m_scDesc.descSetLayout);
    createInfo.pSetLayouts            = &tempLayouts;
    createInfo.pushConstantRangeCount = 1;
    createInfo.pPushConstantRanges    = &pushConstantRanges;
    vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_scanlinePipelineLayout);


    VkShaderModule vertShaderModule = createShaderModule(loadFile("spv/scanline.vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(loadFile("spv/scanline.frag.spv"));

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkVertexInputBindingDescription bindingDescription
        {0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX};

    std::vector<VkVertexInputAttributeDescription> attributeDescriptions {
        {0, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, pos))},
        {1, 0, VK_FORMAT_R32G32B32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, nrm))},
        {2, 0, VK_FORMAT_R32G32_SFLOAT, static_cast<uint32_t>(offsetof(Vertex, texCoord))}};

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        
    vertexInputInfo.vertexAttributeDescriptionCount = attributeDescriptions.size();
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) windowSize.width;
    viewport.height = (float) windowSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = VkExtent2D{windowSize.width, windowSize.height};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE; //??
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;// BEWARE!!  NECESSARY!!
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = m_scanlinePipelineLayout;
    pipelineInfo.renderPass = m_scanlineRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_scanlinePipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create scanline pipeline!");
    }

    // Done with the temporary spv shader modules.
    vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    
    // @@ To destroy:  vkDestroyPipelineLayout(m_device, m_scanlinePipelineLayout, nullptr);
    // @@  and:        vkDestroyPipeline(m_device, m_scanlinePipeline, nullptr);
}*/

void VkApp::createScPipeline()
{
    vk::PushConstantRange pushConstantRanges = {
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0, sizeof(PushConstantRaster) };

    // Creating the Pipeline Layout
    vk::PipelineLayoutCreateInfo createInfo;
    createInfo.setSetLayoutCount(1);
    createInfo.setPSetLayouts(&m_scDesc.descSetLayout);
    createInfo.setPushConstantRangeCount(1);
    createInfo.setPPushConstantRanges(&pushConstantRanges);
    m_device.createPipelineLayout(&createInfo, nullptr, &m_scanlinePipelineLayout);


    vk::ShaderModule vertShaderModule = createShaderModule(loadFile("spv/scanline.vert.spv"));
    vk::ShaderModule fragShaderModule = createShaderModule(loadFile("spv/scanline.frag.spv"));

    vk::PipelineShaderStageCreateInfo vertShaderStageInfo;
    vertShaderStageInfo.setStage(vk::ShaderStageFlagBits::eVertex);
    vertShaderStageInfo.setModule(vertShaderModule);
    vertShaderStageInfo.setPName("main");

    vk::PipelineShaderStageCreateInfo fragShaderStageInfo;
    fragShaderStageInfo.setStage(vk::ShaderStageFlagBits::eFragment);
    fragShaderStageInfo.setModule(fragShaderModule);
    fragShaderStageInfo.setPName("main");

    vk::PipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    vk::VertexInputBindingDescription bindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);


    std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{
        {0, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, pos))},
        {1, 0, vk::Format::eR32G32B32Sfloat, static_cast<uint32_t>(offsetof(Vertex, nrm))},
        {2, 0, vk::Format::eR32G32Sfloat, static_cast<uint32_t>(offsetof(Vertex, texCoord))} };


    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.setVertexBindingDescriptionCount(1);
    vertexInputInfo.setPVertexBindingDescriptions(&bindingDescription);

    vertexInputInfo.setVertexAttributeDescriptionCount(attributeDescriptions.size());
    vertexInputInfo.setPVertexAttributeDescriptions(attributeDescriptions.data());

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.setTopology(vk::PrimitiveTopology::eTriangleList);
    inputAssembly.setPrimitiveRestartEnable(VK_FALSE);

    vk::Viewport viewport;
    viewport.setX(0.0f);
    viewport.setY(0.0f);
    viewport.setWidth((float)windowSize.width);
    viewport.setHeight((float)windowSize.height);
    viewport.setMinDepth(0.0f);
    viewport.setMaxDepth(1.0f);

    vk::Rect2D scissor;
    scissor.setOffset({ 0, 0 });
    scissor.setExtent(vk::Extent2D{ windowSize.width, windowSize.height });

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.setViewportCount(1);
    viewportState.setPViewports(&viewport);
    viewportState.setScissorCount(1);
    viewportState.setPScissors(&scissor);

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.setDepthClampEnable(VK_FALSE);
    rasterizer.setRasterizerDiscardEnable(VK_FALSE);
    rasterizer.setPolygonMode(vk::PolygonMode::eFill);
    rasterizer.setLineWidth(1.0f);
    rasterizer.setCullMode(vk::CullModeFlagBits::eNone); //??
    rasterizer.setFrontFace(vk::FrontFace::eCounterClockwise);
    rasterizer.setDepthBiasEnable(VK_FALSE);

    vk::PipelineMultisampleStateCreateInfo multisampling;
    multisampling.setSampleShadingEnable(VK_FALSE);
    multisampling.setRasterizationSamples(vk::SampleCountFlagBits::e1);

    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    depthStencil.setDepthTestEnable(VK_TRUE);
    depthStencil.setDepthWriteEnable(VK_TRUE);
    depthStencil.setDepthCompareOp(vk::CompareOp::eLessOrEqual);// BEWARE!!  NECESSARY!!
    depthStencil.setDepthBoundsTestEnable(VK_FALSE);
    depthStencil.setStencilTestEnable(VK_FALSE);

    vk::PipelineColorBlendAttachmentState colorBlendAttachment;
    colorBlendAttachment.setColorWriteMask(vk::ColorComponentFlagBits::eR| vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA);
    colorBlendAttachment.setBlendEnable(VK_FALSE);

    vk::PipelineColorBlendStateCreateInfo colorBlending;
    colorBlending.setLogicOpEnable(VK_FALSE);
    colorBlending.setLogicOp(vk::LogicOp::eCopy);
    colorBlending.setAttachmentCount(1);
    colorBlending.setPAttachments(&colorBlendAttachment);
    colorBlending.setBlendConstants({ 0.0f , 0.0f , 0.0f , 0.0f });

    vk::GraphicsPipelineCreateInfo pipelineInfo;
    pipelineInfo.setStageCount(2);
    pipelineInfo.setPStages(shaderStages);
    pipelineInfo.setPVertexInputState(&vertexInputInfo);
    pipelineInfo.setPInputAssemblyState(&inputAssembly);
    pipelineInfo.setPViewportState(&viewportState);
    pipelineInfo.setPRasterizationState(&rasterizer);
    pipelineInfo.setPMultisampleState(&multisampling);
    pipelineInfo.setPDepthStencilState(&depthStencil);
    pipelineInfo.setPColorBlendState(&colorBlending);
    pipelineInfo.setLayout(m_scanlinePipelineLayout);
    pipelineInfo.setRenderPass(m_scanlineRenderPass);
    pipelineInfo.setSubpass(0);
    pipelineInfo.setBasePipelineHandle(VK_NULL_HANDLE);

    if (m_device.createGraphicsPipelines(VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_scanlinePipeline) != vk::Result::eSuccess) {
        throw std::runtime_error("failed to create scanline pipeline!");
    }

    // Done with the temporary spv shader modules.
    //vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
    //vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
    m_device.destroyShaderModule(fragShaderModule);
    m_device.destroyShaderModule(vertShaderModule);
    

    // @@ To destroy:  vkDestroyPipelineLayout(m_device, m_scanlinePipelineLayout, nullptr);
    // @@  and:        vkDestroyPipeline(m_device, m_scanlinePipeline, nullptr);
}

// Create a Vulkan buffer to hold the camera matrices, products and inverses.
// Will be included in a descriptor set for use in shaders.
void VkApp::createMatrixBuffer()
{   
    /*
    m_matrixBW = createBufferWrap(sizeof(MatrixUniforms),
                               VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
                               | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);*/
    m_matrixBW = createBufferWrap(sizeof(MatrixUniforms),
        vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // @@ Destroy with m_matrixBW.destroy(m_device);
}

// Create a Vulkan buffer containing pointers to all object buffers
// (vertex, triangle indices, materials, and material indices. Will be
// included in a descriptor set for use in shaders.
void VkApp::createObjDescriptionBuffer()
{
    //VkCommandBuffer cmdBuf = createTempCmdBuffer();
    vk::CommandBuffer cmdBuf = createTempCppCmdBuffer();
    
    //m_objDescriptionBW  = createStagedBufferWrap(cmdBuf, m_objDesc,
    //                                             VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
    m_objDescriptionBW = createStagedBufferWrap(cmdBuf, m_objDesc, vk::BufferUsageFlagBits::eStorageBuffer);

    //submitTempCmdBuffer(cmdBuf);
    submitTemptCppCmdBuffer(cmdBuf);

    // @@ Destroy with m_objDescriptionBW.destroy(m_device);
}

void VkApp::rasterize()
{
    vk::DeviceSize offset{0};
    
    std::array<vk::ClearValue, 2> clearValues;
    vk::ClearColorValue colorVal;
    colorVal.setFloat32({ 0.0f,0,0,1 });
    clearValues[0].setColor(colorVal);
    clearValues[1].setDepthStencil(vk::ClearDepthStencilValue({ 1.0f, 0 }));

    vk::RenderPassBeginInfo _i;
    _i.setClearValueCount(2);
    _i.setPClearValues(clearValues.data());
    _i.setRenderPass(m_scanlineRenderPass);
    _i.setFramebuffer(m_scanlineFramebuffer);
    _i.renderArea      = {{0, 0}, windowSize };
    m_commandBuffer.beginRenderPass(&_i, vk::SubpassContents::eInline);

    m_commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, m_scanlinePipeline);
    //vkCmdBindDescriptorSets(m_commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
    //                        m_scanlinePipelineLayout, 0, 1, &m_scDesc.descSet, 0, nullptr);
    m_commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_scanlinePipelineLayout, 0, 1,
        &m_scDesc.descSet, 0, nullptr);

    for (const ObjInst& inst : m_objInst) {
        auto& object = m_objData[inst.objIndex];

        // Information pushed at each draw call
        PushConstantRaster pcRaster{
            inst.transform,      // Object's instance transform.
            {0.5f, 2.5f, 3.0f},  // light position;  Should not be hard-coded here!
            inst.objIndex,       // instance Id
            2.5f,                 // light intensity;  Should not be hard-coded here!
            1,                   //Light Type ?
            0.2f                 //Ambient light 
        };

        pcRaster.objIndex = inst.objIndex;  // Telling which object is drawn
        pcRaster.modelMatrix = inst.transform;

        //vkCmdPushConstants(m_commandBuffer, m_scanlinePipelineLayout,
        //                   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
        //                   sizeof(PushConstantRaster), &pcRaster);
        m_commandBuffer.pushConstants(m_scanlinePipelineLayout,
            vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eFragment, 0,
            sizeof(PushConstantRaster), &pcRaster);

        /*
        vkCmdBindVertexBuffers(m_commandBuffer, 0, 1, &object.vertexBuffer.buffer, &offset);
        vkCmdBindIndexBuffer(m_commandBuffer, object.indexBuffer.buffer, 0,
                             VK_INDEX_TYPE_UINT32);
        vkCmdDrawIndexed(m_commandBuffer, object.nbIndices, 1, 0, 0, 0); }
        */
        m_commandBuffer.bindVertexBuffers(0, 1, &object.vertexBuffer.buffer, &offset);
        m_commandBuffer.bindIndexBuffer(object.indexBuffer.buffer, 0, vk::IndexType::eUint32);
        m_commandBuffer.drawIndexed(object.nbIndices, 1, 0, 0, 0);
    }
    
    //vkCmdEndRenderPass(m_commandBuffer);
    m_commandBuffer.endRenderPass();
}


void VkApp::updateCameraBuffer()
{
    // Prepare new UBO contents on host.
    const float    aspectRatio = windowSize.width / static_cast<float>(windowSize.height);
    MatrixUniforms hostUBO     = {};

    glm::mat4    view = app->myCamera.view();
    glm::mat4    proj = app->myCamera.perspective(aspectRatio);
  
    hostUBO.priorViewProj = m_priorViewProj;
    hostUBO.viewProj    = proj * view;
    m_priorViewProj       = hostUBO.viewProj;
    hostUBO.viewInverse = glm::inverse(view);
    hostUBO.projInverse = glm::inverse(proj);

    // UBO on the device, and what stages access it.
    vk::Buffer deviceUBO      = m_matrixBW.buffer;
    auto     uboUsageStages = vk::PipelineStageFlagBits::eVertexShader | 
                              vk::PipelineStageFlagBits::eRayTracingShaderKHR;

    
    // Ensure that the modified UBO is not visible to previous frames.
    vk::BufferMemoryBarrier beforeBarrier;
    beforeBarrier.setSrcAccessMask(vk::AccessFlagBits::eShaderRead);
    beforeBarrier.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);
    beforeBarrier.setBuffer(deviceUBO);
    beforeBarrier.setOffset(0);
    beforeBarrier.setSize(sizeof(hostUBO));

    m_commandBuffer.pipelineBarrier(uboUsageStages, vk::PipelineStageFlagBits::eTransfer, 
                                    vk::DependencyFlagBits::eDeviceGroup, 0, nullptr, 
                                    1, &beforeBarrier, 0, nullptr);

    // Schedule the host-to-device upload. (hostUBO is copied into the cmd
    // buffer so it is okay to deallocate when the function returns).
    m_commandBuffer.updateBuffer(m_matrixBW.buffer, vk::DeviceSize(0), sizeof(MatrixUniforms), &hostUBO);

    // Making sure the updated UBO will be visible.
    vk::BufferMemoryBarrier afterBarrier;
    afterBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite);
    afterBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    afterBarrier.setBuffer(deviceUBO);
    afterBarrier.setOffset(0);
    afterBarrier.setSize(sizeof(hostUBO));
    m_commandBuffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, uboUsageStages,
                                    vk::DependencyFlagBits::eDeviceGroup,
                                    0, nullptr,
                                    1, &afterBarrier, 0, nullptr);
}
