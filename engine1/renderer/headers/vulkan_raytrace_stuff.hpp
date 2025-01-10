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
    // VulkanRaytraceStuff(const VulkanRaytraceStuff&) = delete;
    void init(const char* appname, std::vector<const char *>& requiredInstanceExtensions);
    ~VulkanRaytraceStuff();
    
    std::unique_ptr<vk::Instance> instance;
};
#endif