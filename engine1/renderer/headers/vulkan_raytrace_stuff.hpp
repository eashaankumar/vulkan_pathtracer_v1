#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR
#include <vulkan/vulkan.hpp>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // configures glm to match vulkan's depth range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <SDL2/SDL_vulkan.h>

#include <vector>
#include <memory>

#ifndef VULKAN_RAYTRACE_STUFF
#define VULKAN_RAYTRACE_STUFF
class VulkanRaytraceStuff
{
public:
    void init(const char* appname, int width, int height);
    void run();
    ~VulkanRaytraceStuff();

    struct VulkanBuffer {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::DeviceAddress address;
    };
    
    vk::Extent2D extent;
    SDL_Window* window;
    VulkanBuffer cameraUniformBuffer;
    std::vector<const char *> requiredInstanceExtensions;
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    uint32_t queueId;
    vk::Device device;
    vk::DispatchLoaderDynamic dynamicDispatchLoader;
    vk::Queue computePresentQueue;
    vk::CommandPool commandPool;
    vk::SwapchainKHR swapChain;

    VkSurfaceKHR surface;

    vk::Pipeline rtPipeline;
    vk::PipelineLayout rtPipelineLayout;

    vk::Semaphore semaphore, semaphore2;
    vk::Fence fence;
    std::vector<vk::CommandBuffer> commandBuffers;


    struct Vertex {
        float pos[3];
    };
    
    struct VulkanAccelerationStructure
    {
        vk::AccelerationStructureKHR accelerationStructure;
        VulkanBuffer structureBuffer;
        VulkanBuffer scratchBuffer;
        VulkanBuffer instancesBuffer;
    };

    VulkanAccelerationStructure topAccelerationStructure, bottomAccelerationStructure;

    struct VulkanImage
    {
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView imageView;
    };

    VulkanImage renderTargetImage;

    struct UniformData
    {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    };

    

};

struct Mesh
{
    public:
    std::vector<VulkanRaytraceStuff::Vertex> vertices;
    std::vector<uint32_t> indices;
    VkTransformMatrixKHR transformMatrix;

    VulkanRaytraceStuff::VulkanBuffer vertexBuffer;
    VulkanRaytraceStuff::VulkanBuffer indexBuffer;
    VulkanRaytraceStuff::VulkanBuffer transformBuffer;

    vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress;
    vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress;
    vk::DeviceOrHostAddressConstKHR transformBufferDeviceAddress;
    vk::AccelerationStructureGeometryKHR geometryBLAS;
    
    uint32_t NumTriangles()
    {
        return static_cast<uint32_t>(indices.size() / 3);
    }

    uint32_t IndexCount()
    {
        return static_cast<uint32_t>(indices.size());
    }

    void Prepare(vk::PhysicalDevice& physicalDevice, vk::Device& device);
};
#endif