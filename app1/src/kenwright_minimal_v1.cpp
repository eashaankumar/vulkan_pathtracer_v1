#define _WINDOWS

#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR
#include <vulkan/vulkan.hpp>
#include <shaderc/shaderc.hpp>

// #define GLFW_INCLUDE_VULKAN
// #include <GLFW/glfw3.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>

#include <iostream>
#include <set>
#include <fstream>
#include <string>
#include <random>
#include <functional>
#include <vector>
#include <chrono>
#include <thread>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // configures glm to match vulkan's depth range
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#ifndef DBG_ASSERT
#if defined(_WINDOWS)
#define DBG_ASSERT(f) {if (!(f)){__debugbreak();};}
#else
#define DBG_ASSERT(f) {#error(platform assert todo)}
#endif
#endif

#define VK_CHECK_RESULT(f) {DBG_ASSERT(f==vk::Result(0));                   }
#define DBG_ASSERT_WARN(f,w) { {if(!f){std::cout << w;}}; DBG_ASSERT(f);    }

#include "kenwright_minimal_v1.hpp"

#ifdef KENWRIGHT_MINIMAL_V1

#define APP_NAME "Vulkan Ray Minimal Abstraction - Kenwright"

#pragma region Shaders

#pragma region Ray Gen Shader
const std::string raygenShaderCode=R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(binding=0, set=0, rgba8) uniform image2D image;
layout(binding=1, set=0) uniform accelerationStructureEXT topLevelAS;
layout(binding=2, set=0) uniform CameraProperties
{
    mat4 viewInverse;
    mat4 projInverse;
} cam;

layout(location=0) rayPayloadEXT vec3 hitValue;

void main()
{
    const vec2 pixelCenter = vec2(gl_LaunchIDEXT.xy) + vec2(0.5);
    const vec2 inUV = pixelCenter/vec2(gl_LaunchSizeEXT.xy);
    vec2 d = inUV * 2.0 - 1.0;

    vec4 origin = cam.viewInverse * vec4(0,0,0,1);
    vec4 target = cam.projInverse * vec4(d.x, d.y, 1, 1);
    vec4 direction = cam.viewInverse*vec4(normalize(target.xyz),0);
    
    float tmin = 0.001;
    float tmax = 10000.0;

    hitValue = vec3(0.0);

    traceRayEXT(topLevelAS, gl_RayFlagsOpaqueEXT, 0xff, 0, 0, 0, origin.xyz, tmin, direction.xyz, tmax, 0);

    imageStore(image, ivec2(gl_LaunchIDEXT.xy), vec4(hitValue, 0.0));
}
)";
#pragma endregion

#pragma region Miss Shader
const std::string missShaderCode = R"(
#version 460
#extension GL_EXT_ray_tracing : enable

layout(location=0) rayPayloadInEXT vec3 hitValue;

void main()
{
    hitValue = vec3(0.0, 0.0, 0.2);
}

)";
#pragma endregion

#pragma region Closest Hit Shader
const std::string closestHitShaderCode=R"(

#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location=0) rayPayloadInEXT vec3 hitValue;
hitAttributeEXT vec2 attribs;

void main()
{
    const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
    hitValue = barycentricCoords;
}

)";
#pragma endregion

#pragma endregion

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

int KenWrightMinimal_V1::run()
{
    // Init vulkan app info
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName(APP_NAME)
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName("No Engine")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_2);
    
    // Vulkan instance create info
    vk::InstanceCreateInfo instanceCreateInfo;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    SDL_Window* window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640 * 2, 360 * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    
    std::vector<const char *> requiredInstanceExtensions = {};
    uint32_t extensionCount;
    const char** extensionNames = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    extensionNames = new const char *[extensionCount];
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);
    // append all SDL based extensions to total extension name list
    for(int i = 0; i < extensionCount; i++)
    {
        requiredInstanceExtensions.push_back(extensionNames[i]);
    }
    

    instanceCreateInfo.setPApplicationInfo(&appInfo);
    instanceCreateInfo.enabledExtensionCount = (uint32_t)requiredInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();

    vk::Instance instance = vk::createInstance(instanceCreateInfo);

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


    instance.destroy();
    return 0;
}
#endif