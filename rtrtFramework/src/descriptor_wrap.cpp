#include "vkapp.h"
#include "descriptor_wrap.h"
#include <assert.h>

void DescriptorWrap::setBindings(const vk::Device device, std::vector<vk::DescriptorSetLayoutBinding> _bt)
{
    uint maxSets = 1;  // 1 is good enough for us.  In general, may want more;
    bindingTable = _bt;

    // Build descSetLayout
   /* VkDescriptorSetLayoutCreateInfo createInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    createInfo.bindingCount = uint32_t(bindingTable.size());
    createInfo.pBindings = bindingTable.data();
    createInfo.flags = 0;
    createInfo.pNext = nullptr;*/

    vk::DescriptorSetLayoutCreateInfo createInfo;
    createInfo.setBindingCount(uint32_t(bindingTable.size()));
    createInfo.setPBindings(bindingTable.data());

    /*
    VkDescriptorSetLayout descriptorSetLayout;
    vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &descSetLayout);*/

    //vk::DescriptorSetLayout descriptorSetLayout;
    device.createDescriptorSetLayout(&createInfo, nullptr, &descSetLayout);

    // Collect the size required for each descriptorType into a vector of poolSizes
    //std::vector<VkDescriptorPoolSize> poolSizes;
    std::vector<vk::DescriptorPoolSize> poolSizes;

    for (auto it = bindingTable.cbegin(); it != bindingTable.cend(); ++it)  {
        bool found = false;
        for (auto itpool = poolSizes.begin(); itpool != poolSizes.end(); ++itpool) {
            if(itpool->type == it->descriptorType) {
                itpool->descriptorCount += it->descriptorCount * maxSets;
                found = true;
                break; } }
    
        if (!found) {
            //VkDescriptorPoolSize poolSize;
            vk::DescriptorPoolSize poolSize;
            poolSize.setType(it->descriptorType);
            poolSize.setDescriptorCount(it->descriptorCount * maxSets);
            //poolSize.type            = it->descriptorType;
            //poolSize.descriptorCount = it->descriptorCount * maxSets;
            poolSizes.push_back(poolSize); } }

    
    // Build descPool
    /*
    VkDescriptorPoolCreateInfo descrPoolInfo{VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    descrPoolInfo.maxSets                    = maxSets;
    descrPoolInfo.poolSizeCount              = poolSizes.size();
    descrPoolInfo.pPoolSizes                 = poolSizes.data();
    descrPoolInfo.flags                      = 0;
    vkCreateDescriptorPool(device, &descrPoolInfo, nullptr, &descPool);
    */

    vk::DescriptorPoolCreateInfo descrPoolInfo;
    descrPoolInfo.setMaxSets(maxSets);
    descrPoolInfo.setPoolSizeCount(poolSizes.size());
    descrPoolInfo.setPPoolSizes(poolSizes.data());
    device.createDescriptorPool(&descrPoolInfo, nullptr, &descPool);

    // Allocate DescriptorSet
    /*
    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.descriptorPool              = descPool;
    allocInfo.descriptorSetCount          = 1;
    allocInfo.pSetLayouts                 = &descSetLayout;*/
    vk::DescriptorSetAllocateInfo allocInfo;
    allocInfo.setDescriptorPool(descPool);
    allocInfo.setDescriptorSetCount(1);
    allocInfo.setPSetLayouts(&descSetLayout);

    // Warning: The next line creates a single descriptor set from the
    // above pool since that's all our program needs.  This is too
    // restrictive in general, but fine for this program.
    //vkAllocateDescriptorSets(device, &allocInfo, &descSet);
    device.allocateDescriptorSets(&allocInfo, &descSet);
}

void DescriptorWrap::destroy(vk::Device& device)
{
    /*
    vkDestroyDescriptorSetLayout(device, descSetLayout, nullptr);
    vkDestroyDescriptorPool(device, descPool, nullptr);
    */
    device.destroyDescriptorSetLayout(descSetLayout, nullptr);
    device.destroyDescriptorPool(descPool, nullptr);
}

void DescriptorWrap::write(vk::Device& device, uint index, const vk::Buffer& buffer)
{
    //VkDescriptorBufferInfo desBuf{buffer, 0, VK_WHOLE_SIZE};
    vk::DescriptorBufferInfo desBuf;
    desBuf.setBuffer(buffer);
    desBuf.setOffset(0);
    desBuf.setRange(VK_WHOLE_SIZE);

    //VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    
    /*
    writeSet.dstSet          = descSet;
    writeSet.dstBinding      = index;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType  = VkDescriptorType(bindingTable[index].descriptorType);
    writeSet.pBufferInfo = &desBuf;*/
    
    vk::WriteDescriptorSet writeSet;
    writeSet.setDstSet(descSet);
    writeSet.setDstBinding(index);
    writeSet.setDstArrayElement(0);
    writeSet.setDescriptorCount(1);
    writeSet.setDescriptorType(vk::DescriptorType(bindingTable[index].descriptorType));
    writeSet.setPBufferInfo(&desBuf);

    assert(bindingTable[index].binding == index);

    /*
    assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC);*/

    assert(writeSet.descriptorType == vk::DescriptorType::eStorageBuffer ||
            writeSet.descriptorType == vk::DescriptorType::eStorageBufferDynamic ||
            writeSet.descriptorType == vk::DescriptorType::eUniformBuffer ||
            writeSet.descriptorType == vk::DescriptorType::eUniformBufferDynamic);
    
    //vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    device.updateDescriptorSets(1, &writeSet, 0, nullptr);

}

void DescriptorWrap::write(vk::Device& device, uint index, const vk::DescriptorImageInfo& textureDesc)
{
    //VkDescriptorBufferInfo desBuf{nvbuffer.buffer, 0, VK_WHOLE_SIZE};

    /*
    VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeSet.dstSet          = descSet;
    writeSet.dstBinding      = index;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType  = VkDescriptorType(bindingTable[index].descriptorType);
    writeSet.pImageInfo      = &textureDesc;
    */

    vk::WriteDescriptorSet writeSet;
    writeSet.setDstSet(descSet);
    writeSet.setDstBinding(index);
    writeSet.setDstArrayElement(0);
    writeSet.setDescriptorCount(1);
    writeSet.setDescriptorType(vk::DescriptorType(bindingTable[index].descriptorType));
    writeSet.setPImageInfo(&textureDesc);

    assert(bindingTable[index].binding == index);

    /*
    assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE  ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);*/
    assert(writeSet.descriptorType == vk::DescriptorType::eSampler ||
        writeSet.descriptorType == vk::DescriptorType::eCombinedImageSampler||
        writeSet.descriptorType == vk::DescriptorType::eSampledImage ||
        writeSet.descriptorType == vk::DescriptorType::eStorageImage ||
        writeSet.descriptorType == vk::DescriptorType::eInputAttachment);
    
    //vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    device.updateDescriptorSets(1, &writeSet, 0, nullptr);
}
/*
void DescriptorWrap::write(VkDevice& device, uint index, const std::vector<ImageWrap>& textures)
{
    //VkDescriptorBufferInfo desBuf{nvbuffer.buffer, 0, VK_WHOLE_SIZE};
    std::vector<VkDescriptorImageInfo> des;
    for(auto& texture : textures)
        des.emplace_back(texture.Descriptor());

    VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeSet.dstSet          = descSet;
    writeSet.dstBinding      = index;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = des.size();
    writeSet.descriptorType  = bindingTable[index].descriptorType;
    writeSet.pImageInfo = des.data();

    assert(bindingTable[index].binding == index);

    assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE  ||
           writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);
    
    vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
}*/

void DescriptorWrap::write(vk::Device& device, uint index, const std::vector<ImageWrap>& textures)
{
    //VkDescriptorBufferInfo desBuf{nvbuffer.buffer, 0, VK_WHOLE_SIZE};
    //std::vector<VkDescriptorImageInfo> des;
    std::vector<vk::DescriptorImageInfo> des;
    for (auto& texture : textures)
        des.emplace_back(texture.Descriptor());

    /*
    VkWriteDescriptorSet writeSet{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET };
    writeSet.dstSet = descSet;
    writeSet.dstBinding = index;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = des.size();
    writeSet.descriptorType = bindingTable[index].descriptorType;
    writeSet.pImageInfo = des.data();*/

    vk::WriteDescriptorSet writeSet;
    writeSet.setDstSet(descSet);
    writeSet.setDstBinding(index);
    writeSet.setDstArrayElement(0);
    writeSet.setDescriptorCount(des.size());
    writeSet.setDescriptorType(vk::DescriptorType(bindingTable[index].descriptorType));
    writeSet.setPImageInfo(des.data());


    assert(bindingTable[index].binding == index);

    /*
    assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLER ||
        writeSet.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER ||
        writeSet.descriptorType == VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE ||
        writeSet.descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_IMAGE ||
        writeSet.descriptorType == VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT);*/

    assert(writeSet.descriptorType == vk::DescriptorType::eSampler ||
        writeSet.descriptorType == vk::DescriptorType::eCombinedImageSampler || 
        writeSet.descriptorType == vk::DescriptorType::eSampledImage ||
        writeSet.descriptorType == vk::DescriptorType::eStorageImage || 
        writeSet.descriptorType == vk::DescriptorType::eInputAttachment);

    //vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    device.updateDescriptorSets(1, &writeSet, 0, nullptr);
}

void DescriptorWrap::write(vk::Device& device, uint index, const vk::AccelerationStructureKHR& tlas)
{
    /*
    VkWriteDescriptorSetAccelerationStructureKHR descASInfo{
        VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR};
    descASInfo.accelerationStructureCount = 1;
    descASInfo.pAccelerationStructures    = &tlas;*/

    vk::WriteDescriptorSetAccelerationStructureKHR descASInfo;
    descASInfo.setAccelerationStructureCount(1);
    descASInfo.setPAccelerationStructures(&tlas);
  
    /*
    VkWriteDescriptorSet writeSet{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    writeSet.dstSet          = descSet;
    writeSet.dstBinding      = index;
    writeSet.dstArrayElement = 0;
    writeSet.descriptorCount = 1;
    writeSet.descriptorType  = VkDescriptorType(bindingTable[index].descriptorType);
    writeSet.pNext      = &descASInfo;*/

    vk::WriteDescriptorSet writeSet;
    writeSet.setDstSet(descSet);
    writeSet.setDstBinding(index);
    writeSet.setDstArrayElement(0);
    writeSet.setDescriptorCount(1);
    writeSet.setDescriptorType(vk::DescriptorType(bindingTable[index].descriptorType));
    writeSet.setPNext(&descASInfo);

    assert(bindingTable[index].binding == index);

    //assert(writeSet.descriptorType == VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR);

    assert(writeSet.descriptorType == vk::DescriptorType::eAccelerationStructureKHR);
    
    //vkUpdateDescriptorSets(device, 1, &writeSet, 0, nullptr);
    device.updateDescriptorSets(1, &writeSet, 0, nullptr);
}
