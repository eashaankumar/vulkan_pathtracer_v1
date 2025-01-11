#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR
#include <vulkan/vulkan.hpp>

#include <vector>
#include <memory>

#ifndef VULKAN_RAYTRACE_STUFF
#define VULKAN_RAYTRACE_STUFF
class VulkanRaytraceStuff
{
public:
    void init(const char* appname, std::vector<const char *>& requiredInstanceExtensions);
    ~VulkanRaytraceStuff();
    
    vk::Instance instance;
    vk::PhysicalDevice physicalDevice;
    uint32_t queueId;
    vk::Device device;
    vk::DispatchLoaderDynamic dynamicDispatchLoader;
    vk::Queue computePresentQueue;
    vk::CommandPool commandPool;

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

};
#endif