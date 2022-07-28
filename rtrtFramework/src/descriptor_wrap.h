
#pragma once

#include <stdio.h>
#include <string>
#include <vector>

class DescriptorWrap
{
public:
    std::vector<vk::DescriptorSetLayoutBinding> bindingTable;
    
    vk::DescriptorSetLayout descSetLayout;

    vk::DescriptorPool descPool;
    vk::DescriptorSet descSet;// Could be  vector<VkDescriptorSet> for multiple sets;
    
    /*
    void setBindings(const VkDevice device, std::vector<VkDescriptorSetLayoutBinding> _bt);
    void destroy(VkDevice device);
    */

    void setBindings(const vk::Device device, std::vector<vk::DescriptorSetLayoutBinding> _bt);
    void destroy(vk::Device& device);

    // Any data can be written into a descriptor set.  Apparently I need only these few types:
    /*
    void write(VkDevice& device, uint index, const VkBuffer& buffer);
    void write(VkDevice& device, uint index, const VkDescriptorImageInfo& textureDesc);
    void write(VkDevice& device, uint index, const std::vector<ImageWrap>& textures);
    void write(VkDevice& device, uint index, const VkAccelerationStructureKHR& tlas);
    */

    void write(vk::Device& device, uint index, const vk::Buffer& buffer);
    void write(vk::Device& device, uint index, const vk::DescriptorImageInfo& textureDesc);
    void write(vk::Device& device, uint index, const std::vector<ImageWrap>& textures);
    void write(vk::Device& device, uint index, const vk::AccelerationStructureKHR& tlas);
};
