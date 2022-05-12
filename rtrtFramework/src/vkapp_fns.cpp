
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
    vk::ApplicationInfo applicationInfo(appName.c_str(), 1, engineName.c_str(), 1, VK_API_VERSION_1_2);

    // initialize the vk::InstanceCreateInfo
    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo, 
        reqInstanceLayers.size(), reqInstanceLayers.data(),
        reqInstanceExtensions.size(), reqInstanceExtensions.data());

    // create an Instance
    m_instance = vk::createInstance(instanceCreateInfo);

    // @@
    // Verify VkResult is VK_SUCCESS
    // Document with a cut-and-paste of the three list printouts above.
    // To destroy: vkDestroyInstance(m_instance, nullptr);
}

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
    std::cout << "No suitable Physical Device found"<<std::endl;
    assert(false);
}

void VkApp::chooseQueueIndex()
{
    VkQueueFlags requiredQueueFlags = VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_COMPUTE_BIT
                                      | VK_QUEUE_TRANSFER_BIT;
    
    uint32_t mpCount;
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueProperties(mpCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_physicalDevice, &mpCount, queueProperties.data());

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
    
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR};
    
    VkPhysicalDeviceAccelerationStructureFeaturesKHR accelFeature{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR};
    
    VkPhysicalDeviceVulkan13Features features13{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES};
    
    VkPhysicalDeviceVulkan12Features features12{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES};
    
    VkPhysicalDeviceVulkan11Features features11{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES};
    
    VkPhysicalDeviceFeatures2 features2{
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

    // Fill in all structures on the pNext chain
    vkGetPhysicalDeviceFeatures2(m_physicalDevice, &features2);
    // @@
    // Document the whole filled in pNext chain using an api_dump
    // Examine all the many features.  Do any of them look familiar?

    // Turn off robustBufferAccess (WHY?)
    features2.features.robustBufferAccess = VK_FALSE;

    float priority = 1.0;
    VkDeviceQueueCreateInfo queueInfo{VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueInfo.queueFamilyIndex = m_graphicsQueueIndex;
    queueInfo.queueCount       = 1;
    queueInfo.pQueuePriorities = &priority;
    
    VkDeviceCreateInfo deviceCreateInfo{VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO};
    deviceCreateInfo.pNext            = &features2; // This is the whole pNext chain
  
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos    = &queueInfo;
    
    deviceCreateInfo.enabledExtensionCount   = static_cast<uint32_t>(reqDeviceExtensions.size());
    deviceCreateInfo.ppEnabledExtensionNames = reqDeviceExtensions.data();

    vkCreateDevice(m_physicalDevice, &deviceCreateInfo, nullptr, &m_device);
    // @@
    // Verify VK_SUCCESS
    // To destroy: vkDestroyDevice(m_device, nullptr);
}

// That's all for now!
// Many more procedures will follow ...
