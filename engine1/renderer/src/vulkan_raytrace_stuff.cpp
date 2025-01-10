#include "vulkan_raytrace_stuff.hpp"
#include <iostream>
#include <set>

#ifdef VULKAN_RAYTRACE_STUFF

#define _WINDOWS

#ifndef DBG_ASSERT
#if defined(_WINDOWS)
#define DBG_ASSERT(f) {if (!(f)){__debugbreak();};}
#else
#define DBG_ASSERT(f) {#error(platform assert todo)}
#endif
#endif

#define VK_CHECK_RESULT(f) {DBG_ASSERT(f==vk::Result(0));                   }
#define DBG_ASSERT_WARN(f,w) { {if(!f){std::cout << w;}}; DBG_ASSERT(f);    }

#define LOG(f) std::cout << f << std::endl

#pragma region Vulkan Funcs
auto getDeviceProperties = [](const vk::PhysicalDevice& physicalDevice)
{
    vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
    return deviceProperties;
};

auto getRayTracingFeatures = [](const vk::PhysicalDevice& physicalDevice)
{
    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingFeatures = {};
    vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2 = {.pNext=&rayTracingFeatures};
    physicalDevice.getFeatures2(&physicalDeviceFeatures2);
    return rayTracingFeatures;
};

auto getRayTracingProperties = [](const vk::PhysicalDevice& physicalDevice)
{
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingPipelinePropertiesKhr = {};
    vk::PhysicalDeviceProperties2 phyiscalDeviceProperties2 = {.pNext=&rayTracingPipelinePropertiesKhr};
    physicalDevice.getProperties2(&phyiscalDeviceProperties2);
    return rayTracingPipelinePropertiesKhr;
};

auto getAccStructureFeatures = [](const vk::PhysicalDevice& physicalDevice)
{
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accStructureFeatures = {};
    vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2 = {.pNext=&accStructureFeatures};
    physicalDevice.getFeatures2(&physicalDeviceFeatures2);
    return accStructureFeatures;
};

auto getAccStructureProperties = [](const vk::PhysicalDevice& physicalDevice)
{
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR accStructureProperties = {};
    vk::PhysicalDeviceProperties2 physicalDeviceProperties2 = {.pNext=&accStructureProperties};
    physicalDevice.getProperties2(&physicalDeviceProperties2);
    return accStructureProperties;
};

auto getRayQueryFeatures = [](const vk::PhysicalDevice& physicalDevice)
{
    vk::PhysicalDeviceRayQueryFeaturesKHR accRayQueryFeatures = {};
    vk::PhysicalDeviceFeatures2 physicalDeviceFeatures2 = {.pNext=&accRayQueryFeatures};
    physicalDevice.getFeatures2(&physicalDeviceFeatures2);
    return accRayQueryFeatures;
};
#pragma endregion



///
/// helpers
vk::PhysicalDevice getPhysicalDevice(vk::Instance& instance);
uint32_t getQueueId(vk::PhysicalDevice& physicalDevice);
vk::Device createLogicalDevice(uint32_t queueId, vk::PhysicalDevice& physicalDevice);

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

    LOG("Vulkan Raytracing Stuff");
    
    // instance = std::make_unique<vk::Instance>(vk::createInstance(instanceCreateInfo));

    // physicalDevice = std::make_unique<vk::PhysicalDevice>(getPhysicalDevice(*instance));

    // queueId = std::make_unique<uint32_t>(getQueueId(*physicalDevice));

    // LOG(*queueId);

    // try{
        
    //     device = std::make_unique<vk::Device>(createLogicalDevice(*queueId, requiredInstanceExtensions, *physicalDevice));
    // }
    // catch(...)
    // {
    //     std::cerr << "Failed to create logical device" << std::endl;
    // }

    instance = std::make_unique<vk::Instance>(vk::createInstance(instanceCreateInfo));

    // select physical device
    physicalDevice = std::make_unique<vk::PhysicalDevice>(getPhysicalDevice(*instance));

    queueId = std::make_unique<uint32_t>(getQueueId(*physicalDevice));
    LOG("Queue Id: " + *queueId);

    device = std::make_unique<vk::Device>(createLogicalDevice(*queueId, *physicalDevice));
}

/// @brief step 1
vk::PhysicalDevice getPhysicalDevice(vk::Instance& instance)
{
    // Enumerate physical devices
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();

    std::cout << "Number of devices: " << physicalDevices.size() << std::endl;

    const std::vector<const char*> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
    };

    // select physical device
    vk::PhysicalDevice physicalDevice = nullptr;
    for(const vk::PhysicalDevice& d : physicalDevices)
    {
        std::vector<vk::ExtensionProperties> availableExtensions = d.enumerateDeviceExtensionProperties();
        std::set<std::string> requiredExtensions(requiredDeviceExtensions.begin(), requiredDeviceExtensions.end());

        for(const vk::ExtensionProperties& extension : availableExtensions)
        {
            requiredExtensions.erase(extension.extensionName);
        }

        if (requiredExtensions.empty())
        {
            physicalDevice=d;
            break;
        }
    }

    std::cout << "Selected Device: " << getDeviceProperties(physicalDevice).deviceName << std::endl;
    
    std::cout << "Ray tracing supported:" << getRayTracingFeatures(physicalDevice).rayTracingPipeline << std::endl;
    std::cout << "Max Rec. Depth:" << getRayTracingProperties(physicalDevice).maxRayRecursionDepth << std::endl;

    std::cout << "Has Acceleration Structure: " << getAccStructureFeatures(physicalDevice).accelerationStructure << std::endl;
    std::cout << "Has Prim Count: " << getAccStructureProperties(physicalDevice).maxGeometryCount << std::endl;

    std::cout << "Has Ray Query: " << getRayQueryFeatures(physicalDevice).rayQuery << std::endl;

    return physicalDevice;
}

/// @brief Step 2
uint32_t getQueueId(vk::PhysicalDevice& physicalDevice)
{
    std::vector<vk::QueueFamilyProperties> queueFamilies = physicalDevice.getQueueFamilyProperties();
    for(uint32_t i = 0; i < queueFamilies.size(); i++)
    {
        bool supportsGraphics = (queueFamilies[i].queueFlags & vk::QueueFlagBits::eGraphics) == vk::QueueFlagBits::eGraphics;
        bool supportsCompute = (queueFamilies[i].queueFlags & vk::QueueFlagBits::eCompute) == vk::QueueFlagBits::eCompute;

        if (supportsCompute && supportsGraphics)
        {
            return i;
        }
    }
    DBG_ASSERT_WARN(0, "Unable to find a queue that supports both compute and graphic family");
    return (uint32_t)-1;
}

/// @brief Step 3
vk::Device createLogicalDevice(uint32_t queueId, vk::PhysicalDevice& physicalDevice)
{
    const std::vector<const char*> requiredDeviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME,
        VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
        VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
        VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
        VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME,
        VK_KHR_MAINTENANCE3_EXTENSION_NAME
    };

    // create logical device
    float queuePriority = 1.0;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = {
        {.queueFamilyIndex=queueId, .queueCount=1, .pQueuePriorities=&queuePriority}
    };

    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures = {
        .bufferDeviceAddress=true,
        .bufferDeviceAddressCaptureReplay=false,
        .bufferDeviceAddressMultiDevice=false,
    };

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rayTracingPipelineFeatures = {
        .pNext = &bufferDeviceAddressFeatures,
        .rayTracingPipeline=true
    };

    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationStructureFeatures = {
        .pNext = &rayTracingPipelineFeatures,
        .accelerationStructure=true,
        .accelerationStructureCaptureReplay=true,
        .accelerationStructureIndirectBuild=false,
        .accelerationStructureHostCommands=false,
        .descriptorBindingAccelerationStructureUpdateAfterBind=false
    };

    auto deviceCreateInfo = 
        vk::DeviceCreateInfo{
            .pNext=&accelerationStructureFeatures,
            .queueCreateInfoCount=static_cast<uint32_t>(queueCreateInfos.size()),
            .pQueueCreateInfos=queueCreateInfos.data(),
            .enabledExtensionCount=static_cast<uint32_t>(requiredDeviceExtensions.size()),
            .ppEnabledExtensionNames=requiredDeviceExtensions.data(),
            .pEnabledFeatures=nullptr
        };
    
    return physicalDevice.createDevice(deviceCreateInfo);
}

VulkanRaytraceStuff::~VulkanRaytraceStuff()
{
    LOG("Vulkan Raytracing Stuff (Destroy)");

    device->destroy();

    instance->destroy();
}
#endif