
#include <array>
#include <iostream>     // std::cout
#include <fstream>      // std::ifstream

#include <unordered_set>
#include <unordered_map>

#include "vkapp.h"
#include "app.h"
#include "extensions_vk.hpp"


void VkApp::destroyAllVulkanResources()
{
    // @@
    // vkDeviceWaitIdle(m_device);  // Uncomment this when you have an m_device created.

    // Destroy all vulkan objects.
    // ...  All objects created on m_device must be destroyed before m_device.
    //vkDestroyDevice(m_device, nullptr);
    //vkDestroyInstance(m_instance, nullptr);

    // freeing the commandBuffer is optional, 
    // as it will automatically freed when the corresponding CommandPool is
    // destroyed.
    m_device.destroyRenderPass(m_postRenderPass);

    m_depthImage.destroy(m_device);

    m_device.destroySemaphore(m_readSemaphore);
    m_device.destroySemaphore(m_writtenSemaphore);
    m_device.destroyFence(m_waitFence);

    for (auto& imageView : m_imageViews)
    {
        m_device.destroyImageView(imageView);
    }
    m_device.destroySwapchainKHR(m_swapchain);

    m_device.freeCommandBuffers(m_cmdPool, m_commandBuffer);
    m_device.destroyCommandPool(m_cmdPool);
    m_instance.destroySurfaceKHR(m_surface);
    m_device.destroy();
    m_instance.destroy();
}

void VkApp::recreateSizedResources(VkExtent2D size)
{
    assert(false && "Not ready for onResize events.");
    // Destroy everything related to the window size
    // (RE)Create them all at the new size
}
 
void VkApp::createInstance(bool doApiDump)
{
    uint32_t countGLFWextensions{0};
    const char** reqGLFWextensions = glfwGetRequiredInstanceExtensions(&countGLFWextensions);
    // @@
    // Append each GLFW required extension in reqGLFWextensions to reqInstanceExtensions
    // Print them out while your are at it
    assert(countGLFWextensions > 0);
    printf("GLFW required extensions:\n");
    for (unsigned int i = 0; i < countGLFWextensions; i++) {
        reqInstanceExtensions.push_back(reqGLFWextensions[i]);
        std::cout << "\t" << reqGLFWextensions[i] << std::endl;
    }

    // Suggestion: Parse a command line argument to set/unset doApiDump
    if (doApiDump)
        reqInstanceLayers.push_back("VK_LAYER_LUNARG_api_dump");
 
    //Enumerate the layers using the C++ bindings.
    std::vector<vk::LayerProperties> availableLayers = vk::enumerateInstanceLayerProperties();

    // @@
    // Print out the availableLayers
    std::cout << "Instance layers" << std::endl;
    for (auto const& lp : availableLayers)
    {
        std::cout << lp.layerName << ":" << std::endl;
        std::cout << "\tVersion: " << lp.implementationVersion << std::endl;
        std::cout << "\tAPI Version: (" << (lp.specVersion >> 22) << "." << ((lp.specVersion >> 12) & 0x03FF) << "." << (lp.specVersion & 0xFFF) << ")"
            << std::endl;
        std::cout << "\tDescription: " << lp.description << std::endl;
        std::cout << std::endl;
    }

    // @@
    // Print out the availableExtensions

    std::vector<vk::ExtensionProperties> extensionProperties = vk::enumerateInstanceExtensionProperties();

    // sort the extensions alphabetically

    std::sort(extensionProperties.begin(),
        extensionProperties.end(),
        [](vk::ExtensionProperties const& a, vk::ExtensionProperties const& b) { return strcmp(a.extensionName, b.extensionName) < 0; });

    std::cout << "Instance Extensions:" << std::endl;
    for (auto const& ep : extensionProperties)
    {
        std::cout << ep.extensionName << ":" << std::endl;
        std::cout << "\tVersion: " << ep.specVersion << std::endl;
        std::cout << std::endl;
    }

    // initialize the vk::ApplicationInfo structure
    vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, VK_API_VERSION_1_3);

    // initialize the vk::InstanceCreateInfo
    vk::InstanceCreateInfo instanceCreateInfo(
        vk::InstanceCreateFlags(), &applicationInfo,
        reqInstanceLayers.size(), reqInstanceLayers.data(),
        reqInstanceExtensions.size(), reqInstanceExtensions.data());

    // create an Instance
    m_instance = vk::createInstance(instanceCreateInfo);

    // @@
    // Verify VkResult is VK_SUCCESS
    // Document with a cut-and-paste of the three list printouts above.
    // To destroy: vkDestroyInstance(m_instance, nullptr);
}

//Should maybe be renamed to choosePhysicalDevice() ?
void VkApp::createPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> physicalDevices = m_instance.enumeratePhysicalDevices();
    std::vector<uint32_t> compatibleDevices;
  
    printf("%d devices\n", physicalDevices.size());
    vk::PhysicalDeviceProperties GPUproperties;
    std::vector<vk::ExtensionProperties> extensionProperties;
    unsigned int extensionCount;

    // For each GPU:
    for (size_t i = 0; i < physicalDevices.size(); i++) {

        // Get the GPU's properties
        auto GPUproperties2 =
            physicalDevices[i].getProperties2<
            vk::PhysicalDeviceProperties2,
            vk::PhysicalDeviceBlendOperationAdvancedPropertiesEXT>();
        GPUproperties = GPUproperties2.get<vk::PhysicalDeviceProperties2>().properties;
        if (GPUproperties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
            std::cout << "Physical Device : " << GPUproperties.deviceName << 
                " Does not satisfy DISCRETE_GPU property" << std::endl;
            continue;
        }

        std::cout << "Physical Device : " << GPUproperties.deviceName <<
            " Satisfies DISCRETE_GPU property" << std::endl;

        // Get the GPU's extension list;  Another two-step list retrieval procedure:
        extensionProperties =
            physicalDevices[i].enumerateDeviceExtensionProperties();
        std::sort(extensionProperties.begin(),
            extensionProperties.end(),
            [](vk::ExtensionProperties const& a, vk::ExtensionProperties const& b) { return strcmp(a.extensionName, b.extensionName) < 0; });
        
        std::cout << "PhysicalDevice " << GPUproperties.deviceName << " : " << 
            extensionProperties.size() << " extensions:\n";
        for (auto const& ep : extensionProperties)
        {
            std::cout << "\t" << ep.extensionName << ":" << std::endl;
            std::cout << "\t\tVersion: " << ep.specVersion << std::endl;
            std::cout << std::endl;
        }

        extensionCount = 0;
        for (auto const& extensionProperty : extensionProperties) {
            for (auto const &reqExtension : reqDeviceExtensions) {
                if (strcmp(extensionProperty.extensionName, reqExtension) == 0) {
                    extensionCount++;
                    if (extensionCount == reqDeviceExtensions.size()) {
                        m_physicalDevice = physicalDevices[i];
                        return;
                    }
                    break;
                }
            }
        }

        std::cout << "Physical Device : " << GPUproperties.deviceName <<
            " Does not satisfy required extensions" << std::endl;
        // @@ This code is in a loop iterating variable physicalDevice
        // through a list of all physicalDevices.  The
        // physicalDevice's properties (GPUproperties) and a list of
        // its extension properties (extensionProperties) are retrieve
        // above, and here we judge if the physicalDevice (i.e.. GPU)
        // is compatible with our requirements. We consider a GPU to be
        // compatible if it satisfies both:
        
        //    GPUproperties.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
        
        // and
        
        //    All reqDeviceExtensions can be found in the GPUs extensionProperties list
        //      That is: for all i, there exists a j such that:
        //                 reqDeviceExtensions[i] == extensionProperties[j].extensionName

        //  If a GPU is found to be compatible
        //  Return it (physicalDevice), or continue the search and then return the best GPU.
        //    raise an exception of none were found
        //    tell me all about your system if more than one was found.
    }
    throw std::runtime_error("No suitable Physical Device found");
}

void VkApp::chooseQueueIndex()
{
    vk::QueueFlags requiredQueueFlags = vk::QueueFlagBits::eGraphics | 
                                        vk::QueueFlagBits::eCompute | 
                                        vk::QueueFlagBits::eTransfer;

    m_graphicsQueueIndex = -1;
    // need to explicitly specify all the template arguments for getQueueFamilyProperties2 to make the compiler happy
    using Chain = vk::StructureChain<vk::QueueFamilyProperties2, 
                                     vk::QueueFamilyCheckpointPropertiesNV>;
    auto queueFamilyProperties2 = 
        m_physicalDevice.getQueueFamilyProperties2<>();

    
    for (size_t j = 0; j < queueFamilyProperties2.size(); j++)
    {
        std::cout << "\t"
            << "QueueFamily " << j << "\n";
        vk::QueueFamilyProperties const& properties = queueFamilyProperties2[j].queueFamilyProperties;
        std::cout << "\t\t"
            << "QueueFamilyProperties:\n";
        std::cout << "\t\t\t"
            << "queueFlags                  = " << vk::to_string(properties.queueFlags) << "\n";
        std::cout << "\t\t\t"
            << "queueCount                  = " << properties.queueCount << "\n";
        std::cout << "\t\t\t"
            << "timestampValidBits          = " << properties.timestampValidBits << "\n";
        std::cout << "\t\t\t"
            << "minImageTransferGranularity = " << properties.minImageTransferGranularity.width << " x " << properties.minImageTransferGranularity.height
            << " x " << properties.minImageTransferGranularity.depth << "\n";
        std::cout << "\n";

        if (m_graphicsQueueIndex == -1) {
            if (properties.queueFlags & requiredQueueFlags)
                m_graphicsQueueIndex = j;
        }
    }

    if (m_graphicsQueueIndex == -1) {
        throw std::runtime_error("Unable to find a queue with the required flags");
    }
    else {
        std::cout << "Choosing Queue index: " << m_graphicsQueueIndex << std::endl;
    }
    // @@ Use the api_dump to document the results of the above two
    // step.  How many queue families does your Vulkan offer.  Which
    // of them, by index, has the above three required flags?

    //@@ Search the list for (the index of) the first queue family that has the required flags.
    // Verity that your search choose the correct queue family.
    // Record the index in m_graphicsQueueIndex.
    // Nothing to destroy as m_graphicsQueueIndex is just an integer.
    //m_graphicsQueueIndex = you chosen index;
}


void VkApp::createDevice()
{
    // @@
    // Build a pNext chain of the following six "feature" structures:
    //   features2->features11->features12->features13->accelFeature->rtPipelineFeature->NULL

    // Hint: Keep it simple; add a second parameter (the pNext pointer) to each
    // structure point up to the previous structure.
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature;

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelFeature;
    accelFeature.setPNext(&rtPipelineFeature);

    vk::PhysicalDeviceVulkan13Features feature13;
    feature13.setPNext(&accelFeature);

    vk::PhysicalDeviceVulkan12Features feature12;
    feature12.setPNext(&feature13);

    vk::PhysicalDeviceVulkan11Features feature11;
    feature11.setPNext(&feature12);
    
    vk::PhysicalDeviceFeatures2 feature2;
    feature2.setPNext(&feature11);

    m_physicalDevice.getFeatures2(&feature2);
    // Fill in all structures on the pNext chain
    // @@
    // Document the whole filled in pNext chain using an api_dump
    // Examine all the many features.  Do any of them look familiar?

    // Turn off robustBufferAccess (WHY?)
    // features2.features.robustBufferAccess = VK_FALSE;
    // Default argument for feature2 already sets it to VK_FALSE 
    // Hence commented out earlier line

    float priority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(vk::DeviceQueueCreateFlags(),
        static_cast<uint32_t>(m_graphicsQueueIndex), 1, &priority);

    vk::DeviceCreateInfo deviceCreateInfo;
    deviceCreateInfo.setFlags(vk::DeviceCreateFlags());
    
    deviceCreateInfo.setQueueCreateInfoCount(1);
    deviceCreateInfo.setQueueCreateInfos(deviceQueueCreateInfo);
    
    deviceCreateInfo.setEnabledLayerCount(reqInstanceLayers.size());
    deviceCreateInfo.setPpEnabledLayerNames(reqInstanceLayers.data());

    deviceCreateInfo.setEnabledExtensionCount(reqDeviceExtensions.size());
    deviceCreateInfo.setPpEnabledExtensionNames(reqDeviceExtensions.data());

    deviceCreateInfo.setPNext(&feature2);

    m_device = m_physicalDevice.createDevice(deviceCreateInfo);
    // @@
    // Verify VK_SUCCESS
    // To destroy: vkDestroyDevice(m_device, nullptr);
}

// A distribution of individual procedures in vkapp_fns.cpp, starting
// just after the createDevice of the initial distribution, and
// continuing through all procedures needed to achieve the next goal
// "first light".

void VkApp::getCommandQueue()
{
    m_queue = m_device.getQueue(m_graphicsQueueIndex, 0);
    // Returns void -- nothing to verify
    // Nothing to destroy -- the queue is owned by the device.
}

// Calling load_VK_EXTENSIONS from extensions_vk.cpp.  A Python script
// from NVIDIA created extensions_vk.cpp from the current Vulkan spec
// for the purpose of loading the symbols for all registered
// extension.  This be (indistinguishable from) magic.
void VkApp::loadExtensions()
{
    load_VK_EXTENSIONS(m_instance, vkGetInstanceProcAddr, m_device, vkGetDeviceProcAddr);
}

//  VkSurface is Vulkan's name for the screen.  Since GLFW creates and
//  manages the window, it creates the VkSurface at our request.
void VkApp::getSurface()
{
    VkSurfaceKHR _surface;
    VkResult res = glfwCreateWindowSurface(m_instance, app->GLFW_window, nullptr, &_surface);
    if (res == VK_SUCCESS)
        m_surface = vk::SurfaceKHR( _surface );
    else {
        throw std::runtime_error("Unable to create a Surface using GLFW");
    }

    if (m_physicalDevice.getSurfaceSupportKHR(m_graphicsQueueIndex, m_surface) == VK_TRUE)
        return;

    throw std::runtime_error("Could not create a supported surface");
    // @@ Verify VK_SUCCESS from both the glfw... and the vk... calls.
    // @@ Verify isSupported==VK_TRUE, meaning that Vulkan supports presenting on this surface.
    //To destroy: vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
}

// Create a command pool, used to allocate command buffers, which in
// turn are use to gather and send commands to the GPU.  The flag
// makes it possible to reuse command buffers.  The queue index
// determines which queue the command buffers can be submitted to.
// Use the command pool to also create a command buffer.
void VkApp::createCommandPool()
{
    m_cmdPool = m_device.createCommandPool(
                 vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, 
                                            m_graphicsQueueIndex));
    // @@ Verify VK_SUCCESS
    // To destroy: vkDestroyCommandPool(m_device, m_cmdPool, nullptr);

    // Create a command buffer
    m_commandBuffer = m_device.allocateCommandBuffers(
            vk::CommandBufferAllocateInfo(m_cmdPool, vk::CommandBufferLevel::ePrimary, 1)).front();

    // @@ Verify VK_SUCCESS
    // Nothing to destroy -- the pool owns the command buffer.
    // Freeing the allocated command buffer is optional
}

// 
void VkApp::createSwapchain()
{
    vk::SwapchainKHR oldSwapchain = m_swapchain;

    m_device.waitIdle();

    // Get the surface's capabilities
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = 
        m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);

    vk::Extent2D               swapchainExtent;
    if (surfaceCapabilities.currentExtent.width == std::numeric_limits<uint32_t>::max())
    {
        // Does this case ever happen?
        int width, height;
        glfwGetFramebufferSize(app->GLFW_window, &width, &height);

        swapchainExtent.width = std::clamp(swapchainExtent.width,
            surfaceCapabilities.minImageExtent.width,
            surfaceCapabilities.maxImageExtent.width);

        swapchainExtent.height = std::clamp(swapchainExtent.height,
            surfaceCapabilities.minImageExtent.height,
            surfaceCapabilities.maxImageExtent.height);
    }
    else
    {
        // If the surface size is defined, the swap chain size must match
        swapchainExtent = surfaceCapabilities.currentExtent;
    }
    
    // Test against valid size, typically hit when windows are minimized.
    // The app must prevent triggering this code in such a case
    assert(swapchainExtent.width && swapchainExtent.height);
    // @@ If this assert fires, we have some work to do to better deal
    // with the situation.

    // @@  Roll your own two step process to retrieve a list of present mode into
    //    std::vector<VkPresentModeKHR> presentModes;
    //  by making two calls to
    //    vkGetPhysicalDeviceSurfacePresentModesKHR(m_physicalDevice, m_surface, ...);
    // For an example, search above for vkGetPhysicalDeviceQueueFamilyProperties

    // @@ Document your preset modes. I especially want to know if
    // your system offers VK_PRESENT_MODE_MAILBOX_KHR mode.  My
    // high-end windows desktop does; My higher-end Linux laptop
    // doesn't.
    std::vector<vk::PresentModeKHR> present_modes = 
        m_physicalDevice.getSurfacePresentModesKHR(m_surface);

    // Choose VK_PRESENT_MODE_FIFO_KHR as a default (this must be supported)
    vk::PresentModeKHR swapchainPresentMode = vk::PresentModeKHR::eFifo;

    auto present_mode_iter = std::find_if(present_modes.begin(), present_modes.end(),
        [](vk::PresentModeKHR const& _present_mode) { return _present_mode == vk::PresentModeKHR::eMailbox; });

    if (present_mode_iter != present_modes.end()) {
        swapchainPresentMode = *present_mode_iter;
        std::cout << "Mailbox Mode found as a present mode" << std::endl;
    }
    else
        std::cout << "Mailbox Mode NOT found as a present mode" << std::endl;
        

    // @@ But choose VK_PRESENT_MODE_MAILBOX_KHR if it can be found in
    // the retrieved presentModes Several Vulkan tutorials opine that
    // MODE_MAILBOX is the premier mode, but this may not be best for
    // us -- expect more about this later.


    // Get the list of VkFormat's that are supported:
    //@@ Do the two step process to retrieve a list of surface formats in
    //   std::vector<VkSurfaceFormatKHR> formats;
    // with two calls to
    //   vkGetPhysicalDeviceSurfaceFormatsKHR(m_physicalDevice, m_surface, ...);
    // @@ Document the list you get.

    std::vector<vk::SurfaceFormatKHR> formats = 
        m_physicalDevice.getSurfaceFormatsKHR(m_surface);
    assert(!formats.empty());

    auto format_iter = std::find_if(formats.begin(), formats.end(), 
        [](vk::SurfaceFormatKHR const& _format) { return _format.format == vk::Format::eB8G8R8A8Unorm; });

    vk::Format format;
    vk::ColorSpaceKHR colorSpace;
    if (format_iter != formats.end()) {
        format = (*format_iter).format;
        colorSpace = (*format_iter).colorSpace;
    }
    else
        throw std::runtime_error("Unable to find required Format");

    // @@ Replace the above two temporary lines with the following two
    // to choose the first format and its color space as defaults:
    //  VkFormat surfaceFormat = formats[0].format;
    //  VkColorSpaceKHR surfaceColor  = formats[0].colorSpace;

    // @@ Then search the formats (from several lines up) to choose
    // format VK_FORMAT_B8G8R8A8_UNORM (and its color space) if such
    // exists.  Document your list of formats/color-spaces, and your
    // particular choice.

    // Choose the number of swap chain images, within the bounds supported.
    uint imageCount = surfaceCapabilities.minImageCount + 1; // Recommendation: minImageCount+1
    if (surfaceCapabilities.maxImageCount > 0
        && imageCount > surfaceCapabilities.maxImageCount) {
        imageCount = surfaceCapabilities.maxImageCount;
    }

    assert(imageCount == 3);
    // If this triggers, disable the assert, BUT help me understand
    // the situation that caused it.  

    // Create the swap chain
    vk::ImageUsageFlags imageUsage = vk::ImageUsageFlagBits::eColorAttachment | 
                                     vk::ImageUsageFlagBits::eStorage | 
                                     vk::ImageUsageFlagBits::eTransferDst;
    vk::SwapchainCreateInfoKHR swapChainCreateInfo(
        vk::SwapchainCreateFlagsKHR(),
        m_surface,
        imageCount,
        format,
        colorSpace,
        swapchainExtent,
        1,
        imageUsage,
        vk::SharingMode::eExclusive,
        {},
        surfaceCapabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        swapchainPresentMode,
        true,
        nullptr);
    
    m_swapchain = m_device.createSwapchainKHR(swapChainCreateInfo);
    // @@ Verify VK_SUCCESS

    //@@ Do the two step process to retrieve the list of (3) swapchain images
    //   m_swapchainImages (of type std::vector<VkImage>)
    // with two calls to
    //   vkGetSwapchainImagesKHR(m_device, m_swapchain, ...);
    // Verify success
    // Verify and document that you retrieved the correct number of images.
    m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain);
    std::cout << "Image count is : " << imageCount << std::endl;
    std::cout << "Swapchain size is : " << m_swapchainImages.size() << std::endl;
    m_imageCount = imageCount;

    m_barriers.reserve(m_imageCount);
    m_imageViews.reserve(m_imageCount);

    // Create an VkImageView for each swap chain image.

    vk::ImageViewCreateInfo imageViewCreateInfo({}, {}, 
        vk::ImageViewType::e2D, 
        format, {},
        { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });
    for (auto image : m_swapchainImages)
    {
        imageViewCreateInfo.image = image;
        m_imageViews.push_back(m_device.createImageView(imageViewCreateInfo));
    }

    // Create three VkImageMemoryBarrier structures (one for each swap
    // chain image) and specify the desired
    // layout (VK_IMAGE_LAYOUT_PRESENT_SRC_KHR) for each.
    vk::ImageSubresourceRange res_range(vk::ImageAspectFlagBits::eColor, 0,
        VK_REMAINING_MIP_LEVELS, 0, VK_REMAINING_ARRAY_LAYERS);

    for (auto image : m_swapchainImages) {
        vk::ImageMemoryBarrier mem_barrier({}, {},
            vk::ImageLayout::eUndefined, vk::ImageLayout::ePresentSrcKHR,
            {}, {}, image, res_range);
        m_barriers.push_back(mem_barrier);
    }

    // Create a temporary command buffer. submit the layout conversion
    // command, submit and destroy the command buffer.
    //VkCommandBuffer cmd = createTempCmdBuffer();
    vk::CommandBuffer cmd = createTempCppCmdBuffer();
    cmd.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTopOfPipe, 
        vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, m_imageCount, m_barriers.data());

    submitTemptCppCmdBuffer(cmd);

    // Create the three synchronization objects.  These are not
    // technically part of the swap chain, but they are used
    // exclusively for synchronizing the swap chain, so I include them
    // here.
    m_waitFence = m_device.createFence(
            vk::FenceCreateInfo(vk::FenceCreateFlagBits::eSignaled));

    m_readSemaphore = m_device.createSemaphore(vk::SemaphoreCreateInfo());
    m_writtenSemaphore = m_device.createSemaphore(vk::SemaphoreCreateInfo());
    //NAME(m_readSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "m_readSemaphore");
    //NAME(m_writtenSemaphore, VK_OBJECT_TYPE_SEMAPHORE, "m_writtenSemaphore");
    //NAME(m_queue, VK_OBJECT_TYPE_QUEUE, "m_queue");

    windowSize = swapchainExtent;
    // To destroy:  Complete and call function destroySwapchain
}

void VkApp::destroySwapchain()
{
    vkDeviceWaitIdle(m_device);

    // @@
    // Destroy all (3)  m_imageView'Ss with vkDestroyImageView(m_device, imageView, nullptr)

    // Destroy the synchronization items: 
    //  vkDestroyFence(m_device, m_waitFence, nullptr);
    //  vkDestroySemaphore(m_device, m_readSemaphore, nullptr);
    //  vkDestroySemaphore(m_device, m_writtenSemaphore, nullptr);

    // Destroy the actual swapchain with: vkDestroySwapchainKHR(m_device, m_swapchain, nullptr);

    m_swapchain = VK_NULL_HANDLE;
    m_imageViews.clear();
    m_barriers.clear();
}



void VkApp::createDepthResource()
{
    uint mipLevels = 1;

    // Note m_depthImage is type ImageWrap; a tiny wrapper around
    // several related Vulkan objects.
    m_depthImage = createImageWrap(windowSize.width, windowSize.height,
        vk::Format::eX8D24UnormPack32,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        vk::MemoryPropertyFlagBits::eDeviceLocal,
        mipLevels);
    m_depthImage.imageView = createImageView(m_depthImage.image,
        vk::Format::eX8D24UnormPack32,
        vk::ImageAspectFlagBits::eDepth);
    // To destroy: m_depthImage.destroy(m_device);
}

// Gets a list of memory types supported by the GPU, and search
// through that list for one that matches the requested properties
// flag.  The (only?) two types requested here are:
//
// (1) VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT: For the bulk of the memory
// used by the GPU to store things internally.
//
// (2) VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT:
// for memory visible to the CPU  for CPU to GPU copy operations.
uint32_t VkApp::findMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties)
{
    vk::PhysicalDeviceMemoryProperties memoryProperties = m_physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((typeBits & (1 << i)) &&
            ((memoryProperties.memoryTypes[i].propertyFlags & properties) == properties))
        {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}


// A factory function for an ImageWrap, this creates a VkImage and
// creates and binds an associated VkDeviceMemory object.  The
// VkImageView and VkSampler are left empty to be created elsewhere as
// needed.
ImageWrap VkApp::createImageWrap(uint32_t width, uint32_t height,
    vk::Format format,
    vk::ImageUsageFlags usage,
    vk::MemoryPropertyFlags properties, uint mipLevels)
{
    ImageWrap myImage;

    vk::ImageCreateInfo imageCreateInfo(vk::ImageCreateFlags(),
        vk::ImageType::e2D,
        format,
        vk::Extent3D(width, height, 1),
        mipLevels,
        1,
        vk::SampleCountFlagBits::e1,
        vk::ImageTiling::eOptimal,
        usage);
    myImage.image = m_device.createImage(imageCreateInfo);

    vk::PhysicalDeviceMemoryProperties memoryProperties = m_physicalDevice.getMemoryProperties();
    vk::MemoryRequirements memoryRequirements = m_device.getImageMemoryRequirements(myImage.image);
    uint32_t typeIndex = findMemoryType(memoryRequirements.memoryTypeBits, properties);

    myImage.memory = m_device.allocateMemory(
        vk::MemoryAllocateInfo(memoryRequirements.size, typeIndex));

    m_device.bindImageMemory(myImage.image, myImage.memory, 0);

    myImage.imageView = VK_NULL_HANDLE;
    myImage.sampler = VK_NULL_HANDLE;

    return myImage;
    // @@ Verify success for vkCreateImage, and vkAllocateMemory
}

VkImageView VkApp::createImageView(vk::Image image, vk::Format format,
    vk::ImageAspectFlags aspect)
{
    // @@ Verify success for vkCreateImageView
    vk::ImageView imageView = m_device.createImageView(
        vk::ImageViewCreateInfo(
            vk::ImageViewCreateFlags(), 
            image, 
            vk::ImageViewType::e2D, 
            format, {}, 
            { aspect, 0, 1, 0, 1 }));

    return imageView;
}

void VkApp::createPostRenderPass()
{
    std::array<vk::AttachmentDescription, 2> attachments{};
    // Color attachment
    attachments[0].setFormat(vk::Format::eB8G8R8A8Unorm);
    attachments[0].setLoadOp(vk::AttachmentLoadOp::eClear);
    attachments[0].setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
    attachments[0].setSamples(vk::SampleCountFlagBits::e1);

    // Depth attachment
    attachments[1].setFormat(vk::Format::eX8D24UnormPack32);
    attachments[1].setLoadOp(vk::AttachmentLoadOp::eClear);
    attachments[1].setStencilLoadOp(vk::AttachmentLoadOp::eClear);
    attachments[1].setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
    attachments[1].setSamples(vk::SampleCountFlagBits::e1);

    const vk::AttachmentReference colorReference(0, vk::ImageLayout::eColorAttachmentOptimal);
    const vk::AttachmentReference depthReference(1, vk::ImageLayout::eDepthStencilAttachmentOptimal);


    std::array<vk::SubpassDependency, 1> subpassDependencies;
    // Transition from final to initial (VK_SUBPASS_EXTERNAL refers to all commands executed outside of the actual renderpass)
    subpassDependencies[0].setSrcSubpass(VK_SUBPASS_EXTERNAL);
    subpassDependencies[0].setDstSubpass(0);
    subpassDependencies[0].setSrcStageMask(vk::PipelineStageFlagBits::eBottomOfPipe);
    subpassDependencies[0].setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput);
    subpassDependencies[0].setSrcAccessMask(vk::AccessFlagBits::eMemoryRead);
    subpassDependencies[0].setDstAccessMask(
        vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite);
    subpassDependencies[0].setDependencyFlags(vk::DependencyFlagBits::eByRegion);

    vk::SubpassDescription subpassDescription;
    subpassDescription.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics);
    subpassDescription.setColorAttachmentCount(1);
    subpassDescription.setColorAttachments(colorReference);
    subpassDescription.setPDepthStencilAttachment(&depthReference);

    vk::RenderPassCreateInfo renderPassInfo;
    renderPassInfo.setAttachmentCount(attachments.size());
    renderPassInfo.setPAttachments(attachments.data());
    renderPassInfo.setSubpassCount(1);
    renderPassInfo.setPSubpasses(&subpassDescription);
    renderPassInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassInfo.setDependencyCount(subpassDependencies.size());
    renderPassInfo.setPDependencies(subpassDependencies.data());

    m_postRenderPass = m_device.createRenderPass(renderPassInfo);
    // To destroy: vkDestroyRenderPass(m_device, m_postRenderPass, nullptr);
}

// A VkFrameBuffer wraps several images into a render target --
// usually a color buffer and a depth buffer.
void VkApp::createPostFrameBuffers()
{
    std::array<VkImageView, 2> fbattachments{};

    // Create frame buffers for every swap chain image
    VkFramebufferCreateInfo _ci{ VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
    _ci.renderPass = m_postRenderPass;
    _ci.width = windowSize.width;
    _ci.height = windowSize.height;
    _ci.layers = 1;
    _ci.attachmentCount = 2;
    _ci.pAttachments = fbattachments.data();

    // Each of the three swapchain images gets an associated frame
    // buffer, all sharing one depth buffer.
    m_framebuffers.resize(m_imageCount);
    for (uint32_t i = 0; i < m_imageCount; i++) {
        fbattachments[0] = m_imageViews[i];         // A color attachment from the swap chain
        fbattachments[1] = m_depthImage.imageView;  // A depth attachment
        vkCreateFramebuffer(m_device, &_ci, nullptr, &m_framebuffers[i]);
    }

    // To destroy: In a loop, call: vkDestroyFramebuffer(m_device, m_framebuffers[i], nullptr);
    // Verify success
}


void VkApp::createPostPipeline()
{

    // Creating the pipeline layout
    VkPipelineLayoutCreateInfo createInfo{ VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };

    // What we eventually want:
    //createInfo.setLayoutCount         = 1;
    //createInfo.pSetLayouts            = &m_scDesc.descSetLayout;
    // Push constants in the fragment shader
    //VkPushConstantRange pushConstantRanges = {VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(float)};
    //createInfo.pushConstantRangeCount = 1;
    //createInfo.pPushConstantRanges    = &pushConstantRanges;

    // What we can do now as a first pass:
    createInfo.setLayoutCount = 0;
    createInfo.pSetLayouts = nullptr;
    createInfo.pushConstantRangeCount = 0;
    createInfo.pPushConstantRanges = nullptr;

    vkCreatePipelineLayout(m_device, &createInfo, nullptr, &m_postPipelineLayout);

    ////////////////////////////////////////////
    // Create the shaders
    ////////////////////////////////////////////
    VkShaderModule vertShaderModule = createShaderModule(loadFile("spv/post.vert.spv"));
    VkShaderModule fragShaderModule = createShaderModule(loadFile("spv/post.frag.spv"));

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

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    //auto bindingDescription = Vertex::getBindingDescription();
    //auto attributeDescriptions = Vertex::getAttributeDescriptions();

    // No geometry in this pipeline's draw.
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)windowSize.width;
    viewport.height = (float)windowSize.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = VkExtent2D{ windowSize.width, windowSize.height };

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
    pipelineInfo.layout = m_postPipelineLayout;
    pipelineInfo.renderPass = m_postRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr,
        &m_postPipeline);

    // The pipeline has fully compiled copies of the shaders, so these
    // intermediate (SPV) versions can be destroyed.
    // @@
    // For the two modules fragShaderModule and vertShaderModule
    // destroy right *here* via vkDestroyShaderModule(m_device, ..., nullptr);

    // To destroy:  vkDestroyPipelineLayout(m_device, m_postPipelineLayout, nullptr);
    //  and:        vkDestroyPipeline(m_device, m_postPipeline, nullptr);
    // Document the vkCreateGraphicsPipelines call with an api_dump.  

}

std::string VkApp::loadFile(const std::string& filename)
{
    std::string   result;
    std::ifstream stream(filename, std::ios::ate | std::ios::binary);  //ate: Open at file end

    if (!stream.is_open())
        return result;

    result.reserve(stream.tellg()); // tellg() is last char position in file (i.e.,  length)
    stream.seekg(0, std::ios::beg);

    result.assign((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    return result;
}

// That's all for now!
// Many more procedures will follow ...
