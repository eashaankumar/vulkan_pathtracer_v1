#include "kenwright_1_2.hpp"
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <vector>

#ifdef KENWRIGHT_1_2
int Kenwright_1_2::run()
{
    // Init vulkan app info
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName("Vulkan Ray Tracing Check")
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName("No Engine")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_2);
    
    // Vulkan instance create info
    vk::InstanceCreateInfo createInfo;
    createInfo.setPApplicationInfo(&appInfo);

    // Create Vulkan instance
    vk::Instance instance;
    try
    {
        instance = vk::createInstance(createInfo);
    }
    catch (const vk::SystemError& err)
    {
        std::cerr << "Failed to create Vulkan instance: " << err.what() << std::endl;
        return -1;
    }

    // enumerate physical devices available on the system
    std::vector<vk::PhysicalDevice> devices = instance.enumeratePhysicalDevices();
    if (devices.empty())
    {
        std::cerr << "No GPUs with vulkan support found" << std::endl;
        instance.destroy();
        return -1;
    }

    // Check each device for ray-tracing support
    bool rayTracingSupport = false;
    for(const auto& device : devices)
    {
        std::vector<vk::ExtensionProperties> extensions = device.enumerateDeviceExtensionProperties();

        // Flag to check for ray-tracing specific extensions
        bool hasAccelerationStructure = false;
        bool hasRayTracingPipeline = false;
        
        for(const auto& extension : extensions)
        {
            if (strcmp(extension.extensionName, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME) == 0)
            {
                hasAccelerationStructure = true;
            }
            if (strcmp(extension.extensionName, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME) == 0)
            {
                hasRayTracingPipeline = true;
            }
        }

        if(hasRayTracingPipeline && hasAccelerationStructure)
        {
            rayTracingSupport = true;
            break;
        }
    }

    if (rayTracingSupport)
    {
        std::cout << "Ray tracing is supported on this device." << std::endl;
    }
    else{
        std::cout << "Ray tracing is not supported on this device." << std::endl;
    }

    // Clean up vulkan instance
    instance.destroy();
    return 0;
}

#endif