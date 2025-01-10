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
    
    std::unique_ptr<vk::Instance> instance;
    std::unique_ptr<vk::PhysicalDevice> physicalDevice;
    std::unique_ptr<uint32_t> queueId;
    std::unique_ptr<vk::Device> device;
};
#endif