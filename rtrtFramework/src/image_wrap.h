#include <vulkan/vulkan.hpp>

# pragma once

struct ImageWrap
{
    vk::Image          image;
    vk::DeviceMemory   memory;
    vk::Sampler        sampler;
    vk::ImageView      imageView;
    vk::ImageLayout    imageLayout;
    
    void destroy(vk::Device device)
    {
        device.destroyImage(image);
        device.destroyImageView(imageView);
        device.freeMemory(memory);
        device.destroySampler(sampler);
    }
    
    vk::DescriptorImageInfo Descriptor() const 
    {
        return vk::DescriptorImageInfo({sampler, imageView, imageLayout});
    }
};
