# pragma once

struct BufferWrap
{
    //VkBuffer buffer;
    vk::Buffer buffer;
    //VkDeviceMemory memory;
    vk::DeviceMemory memory;
    
    void destroy(VkDevice& device)
    {
        vkDestroyBuffer(device, buffer, nullptr);
        vkFreeMemory(device, memory, nullptr);
    }

    void destroy(vk::Device& device)
    {
        device.destroyBuffer(buffer);
        device.freeMemory(memory);
    }

};
