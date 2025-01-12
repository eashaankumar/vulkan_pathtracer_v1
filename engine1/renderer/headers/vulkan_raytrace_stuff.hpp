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
    ~VulkanRaytraceStuff();
    
    SDL_Window* window;
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
    
    struct VulkanBuffer {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::DeviceAddress address;
    };

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
#endif