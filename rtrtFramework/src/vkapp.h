
#pragma once

#include <algorithm>
#include "vulkan/vulkan_core.h"
#include <vulkan/vulkan.hpp>  // A modern C++ API for Vulkan. Beware 14K lines of code

// Imgui
#define GUI
#ifdef GUI
#include "backends/imgui_impl_glfw.h"
#include "imgui.h"
#include "backends/imgui_impl_vulkan.h"
#endif

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#endif
#include "GLFW/glfw3.h"
#include "GLFW/glfw3native.h"

#include "shaders/shared_structs.h"
#include "buffer_wrap.h"
#include "image_wrap.h"
#include "descriptor_wrap.h"
#include "acceleration_wrap.h"

//#include "raytracing_wrap.h"
#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include <glm/glm.hpp>
//#include <vulkan/vulkan.hpp>
// The OBJ model
struct ObjData
{
    uint32_t     nbIndices{0};
    uint32_t     nbVertices{0};
    glm::mat4 transform;      // Instance matrix of the object
    BufferWrap vertexBuffer;    // Device buffer of all 'Vertex'
    BufferWrap indexBuffer;     // Device buffer of the indices forming triangles
    BufferWrap matColorBuffer;  // Device buffer of array of 'Wavefront material'
    BufferWrap matIndexBuffer;  // Device buffer of array of 'Wavefront material'
    BufferWrap lightBuffer;     // Device buffer of all the light triangle indeces.

    void destroy(vk::Device& device) {
        vertexBuffer.destroy(device);
        indexBuffer.destroy(device);
        matColorBuffer.destroy(device);
        matIndexBuffer.destroy(device);
        lightBuffer.destroy(device);
    }
};

struct ObjInst
{
    glm::mat4 transform;    // Matrix of the instance
    uint32_t  objIndex;     // Model index
};

class App;

class VkApp
{
private:
    std::string appName = "599-RTRT";
    std::string engineName = "no-name";
public:
    std::vector<const char*> reqInstanceExtensions = {  // GLFW will add to this
        "VK_EXT_debug_utils"                            // Can help debug validation errors
    };
    
    std::vector<const char*> reqInstanceLayers = {
        //"VK_LAYER_LUNARG_api_dump",  // Useful, but prefer to add (or not) at runtime
        "VK_LAYER_KHRONOS_validation"  // ALWAYS!!
    };
    
    std::vector<const char*> reqDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,		 // Presentation engine; draws to screen
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,	 // Ray tracing extension
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,	 // Ray tracing extension
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME}; // Required by ray tracing pipeline;
    
    App* app;
    VkApp(App* _app);

    void drawFrame();

    void destroyAllVulkanResources();

    // Some auxiliary functions
    void recreateSizedResources(VkExtent2D size);
    VkCommandBuffer createTempCmdBuffer();
    vk::CommandBuffer createTempCppCmdBuffer();
    void submitTempCmdBuffer(VkCommandBuffer cmdBuffer);
    void submitTemptCppCmdBuffer(vk::CommandBuffer cmdBuffer);
    vk::ShaderModule createShaderModule(std::string code);
    VkPipelineShaderStageCreateInfo createShaderStageInfo(const std::string&    code,
                                                          VkShaderStageFlagBits stage,
                                                          const char* entryPoint = "main");
                            
    // Vulkan objects that will be created and the functions that will do the creation.
    vk::Instance m_instance;
    void createInstance(bool doApiDump);

    vk::PhysicalDevice m_physicalDevice;
    void createPhysicalDevice();

    uint32_t m_graphicsQueueIndex{VK_QUEUE_FAMILY_IGNORED};
    void chooseQueueIndex();

    vk::Device m_device;
    void createDevice();

    vk::Queue m_queue;
    void getCommandQueue();
    
    void loadExtensions();

    vk::SurfaceKHR m_surface;
    void getSurface();
    
    vk::CommandPool m_cmdPool;
    vk::CommandBuffer m_commandBuffer;
    void createCommandPool();

    vk::SwapchainKHR m_swapchain;
    uint32_t       m_imageCount{0};
    std::vector<vk::Image>     m_swapchainImages;  // from vkGetSwapchainImagesKHR
    std::vector<vk::ImageView> m_imageViews{};
    std::vector<vk::ImageMemoryBarrier> m_barriers{};  // Filled in  VkImageMemoryBarrier objects
 
    vk::Fence m_waitFence;
    vk::Semaphore m_readSemaphore;
    vk::Semaphore m_writtenSemaphore{};
    vk::Extent2D windowSize{0, 0}; // Size of the window
    void createSwapchain();
    void destroySwapchain();

    ImageWrap m_depthImage;
    void createDepthResource();
    
    vk::PipelineLayout m_postPipelineLayout;
    vk::RenderPass m_postRenderPass;
    void createPostRenderPass();
    
    std::vector<VkFramebuffer> m_framebuffers{}; // One frambuffer per swapchain image.
    void createPostFrameBuffers();

    vk::Pipeline m_postPipeline;
    void createPostPipeline();
    
    #ifdef GUI
    VkDescriptorPool m_imguiDescPool{VK_NULL_HANDLE};
    void initGUI();
    #endif
    
    vk::RenderPass m_scanlineRenderPass;
    vk::Framebuffer m_scanlineFramebuffer;
    void createScanlineRenderPass();

    ImageWrap m_scImageBuffer{};
    void createScBuffer();
    
    ImageWrap m_rtColCurrBuffer{}; 
    ImageWrap m_rtColPrevBuffer{};
    ImageWrap m_rtNDCurrBuffer{};
    ImageWrap m_rtNDPrevBuffer{};
    ImageWrap m_rtKdCurrBuffer{};
    void createRtBuffers();
    
    void cmdCopyImage(ImageWrap& src, ImageWrap& dst);

    ImageWrap m_denoiseBuffer{};
    void createDenoiseBuffer();

    // Arrays of objects instances and textures in the scene
    std::vector<ObjData>  m_objData{};  // Obj data in Vulkan Buffers
    std::vector<ObjDesc>  m_objDesc{};  // Device-addresses of those buffers
    std::vector<ImageWrap>  m_objText{};  // All textures of the scene
    std::vector<ObjInst>  m_objInst{};  // Instances paring an object and a transform
    void myloadModel(const std::string& filename, glm::mat4 transform);

    BufferWrap m_objDescriptionBW{};  // Device buffer of the OBJ descriptions
    void createObjDescriptionBuffer();

    DescriptorWrap m_scDesc{};
    void createScDescriptorSet();

    vk::PipelineLayout            m_scanlinePipelineLayout;
    vk::Pipeline                  m_scanlinePipeline;
    void createScPipeline();

    BufferWrap m_matrixBW{};  // Device-Host of the camera matrices
    void createMatrixBuffer();
    
    BufferWrap m_scratch1;
    BufferWrap m_scratch2;

    RaytracingBuilderKHR m_rtBuilder{};

    float m_maxAnis = 0;
    PushConstantRay m_pcRay{};  // Push constant for ray tracer
    int m_num_atrous_iterations = 0;
    PushConstantDenoise m_pcDenoise{};
    uint32_t handleSize{};
    uint32_t handleAlignment{};
    uint32_t baseAlignment{};
    void initRayTracing();

    // // Accelleration structure objects and functions

    BlasInput objectToVkGeometryKHR(const ObjData& model);
    void createBottomLevelAS();
    void createTopLevelAS();
    void createRtAccelerationStructure();

    DescriptorWrap m_rtDesc{};
    void createRtDescriptorSet();

    vk::PipelineLayout                                  m_rtPipelineLayout;
    vk::Pipeline                                        m_rtPipeline;
    void createRtPipeline();
    
    BufferWrap m_shaderBindingTableBW;
    vk::StridedDeviceAddressRegionKHR m_rgenRegion{};
    vk::StridedDeviceAddressRegionKHR m_missRegion{};
    vk::StridedDeviceAddressRegionKHR m_hitRegion{};
    vk::StridedDeviceAddressRegionKHR m_callRegion{};
    void createRtShaderBindingTable();

    DescriptorWrap m_postDesc{};
    void createPostDescriptor();

    DescriptorWrap m_denoiseDesc{};
    void createDenoiseDescriptorSet();
    
    vk::PipelineLayout            m_denoiseCompPipelineLayout;
    vk::Pipeline                  m_denoisePipelineX, m_denoisePipelineY;
    void createDenoiseCompPipeline();

    // Run loop 
    bool useRaytracer = true;
    bool specularOn = true;
    bool specularToggled = false;

    void prepareFrame();
    void ResetRtAccumulation();
    
    BufferWrap m_lightBuff{};

    glm::mat4 m_priorViewProj{};
    void updateCameraBuffer();
    void rasterize();
    void raytrace();
    void denoise();
    
    uint32_t m_swapchainIndex{0};
    
    void postProcess();
    void submitFrame();

       
    
    std::string loadFile(const std::string& filename);
    uint32_t findMemoryType(uint32_t typeBits, vk::MemoryPropertyFlags properties);
    
    BufferWrap createStagedBufferWrap(const vk::CommandBuffer& cmdBuf,
                                      const vk::DeviceSize&    size,
                                      const void*            data,
                                      vk::BufferUsageFlags     usage);
    template <typename T>
    BufferWrap createStagedBufferWrap(const vk::CommandBuffer& cmdBuf,
                                      const std::vector<T>&  data,
                                      vk::BufferUsageFlags     usage)
    {
        return createStagedBufferWrap(cmdBuf, sizeof(T)*data.size(), data.data(), usage);
    }
    

    BufferWrap createBufferWrap(vk::DeviceSize size, vk::BufferUsageFlags usage,
                                vk::MemoryPropertyFlags properties);

     void copyBuffer(vk::Buffer srcBuffer, vk::Buffer dstBuffer, vk::DeviceSize size);
    
    
    void transitionImageLayout(vk::Image image,
                                vk::Format format,
                                vk::ImageLayout oldLayout,
                                vk::ImageLayout newLayout,
                                uint32_t mipLevels=1);

    void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height);
    
    ImageWrap createTextureImage(std::string fileName);
    ImageWrap createBufferImage(vk::Extent2D& size);
    
    ImageWrap createImageWrap(uint32_t width, uint32_t height,
                              vk::Format format, vk::ImageUsageFlags usage,
                              vk::MemoryPropertyFlags properties, uint mipLevels = 1);

    VkImageView createImageView(vk::Image image, vk::Format format,
                                vk::ImageAspectFlags aspect);
    VkSampler createTextureSampler();
    
    void generateMipmaps(VkImage image, VkFormat imageFormat,
                         int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void generateMipmaps(vk::Image image, vk::Format imageFormat,
                         int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

    void imageLayoutBarrier(vk::CommandBuffer cmdbuffer,
        vk::Image image,
        vk::ImageLayout oldImageLayout,
        vk::ImageLayout newImageLayout,
        vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor);
};
