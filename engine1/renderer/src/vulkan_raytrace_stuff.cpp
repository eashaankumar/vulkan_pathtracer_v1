#include "vulkan_raytrace_stuff.hpp"
#include <iostream>

#ifdef VULKAN_RAYTRACE_STUFF
void VulkanRaytraceStuff::init(const char* appname, std::vector<const char *>& requiredInstanceExtensions )
{
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName(appname)
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName("Engine1")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_2);

    vk::InstanceCreateInfo instanceCreateInfo;
    instanceCreateInfo.setPApplicationInfo(&appInfo);
    instanceCreateInfo.enabledExtensionCount = (uint32_t)requiredInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

    std::cout << "Vulkan Raytracing Stuff" << std::endl;

    instance = std::make_unique<vk::Instance>(vk::createInstance(instanceCreateInfo));
}

VulkanRaytraceStuff::~VulkanRaytraceStuff()
{
    std::cout << "Vulkan Raytracing Stuff (Destroy)" << std::endl;
    instance->destroy();
}
#endif