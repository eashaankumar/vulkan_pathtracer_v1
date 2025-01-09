#define _WINDOWS

#define VULKAN_HPP_NO_CONSTRUCTORS
#define VULKAN_HPP_NO_STRUCT_CONSTRUCTORS
#define VULKAN_HPP_HAS_SPACESHIP_OPERATOR
#include <vulkan/vulkan.hpp>
#pragma comment(lib, "vulkan-1.lib")

// #include <shaderc/shaderc.hpp> // Takes too long to link! And doesnt work...
// #pragma comment(lib, "shadercd.lib")

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

#define WINDOW_WIDTH 640 * 2
#define WINDOW_HEIGHT 360 * 2
#define LOG(x) std::cout <<(x) << std::endl

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

int KenWrightMinimal_V1::run()
{
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
    SDL_Window* window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    
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

    uint32_t queueId = [&physicalDevice]()
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
    }();

    //std::cout << queueId << std::endl;

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
    
    vk::Device device=physicalDevice.createDevice(deviceCreateInfo);

    // Dynamic Dispatching
    vk::DispatchLoaderDynamic dynamicDispatchLoader = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr, device);

    vk::Queue computePresentQueue = device.getQueue(queueId, 0);

    vk::CommandPool commandPool = device.createCommandPool({.queueFamilyIndex=queueId});

    auto findMemoryTypeIndex = [&physicalDevice](const uint32_t& memoryTypeBits, const vk::MemoryPropertyFlags& properties)
    {
        vk::PhysicalDeviceMemoryProperties memoryProperties = physicalDevice.getMemoryProperties();

        for(uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
        {
            if ((memoryTypeBits & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                return i;
            }
        }
        DBG_ASSERT_WARN(0, "Unable to find suitable memory type!");
        return uint32_t(0);
    };

    struct VulkanBuffer {
        vk::Buffer buffer;
        vk::DeviceMemory memory;
        vk::DeviceAddress address;
    };

    auto createBuffer = [&findMemoryTypeIndex, &physicalDevice, &device] (const vk::DeviceSize& size,
        const vk::Flags<vk::BufferUsageFlagBits>& usage,
        const vk::Flags<vk::MemoryPropertyFlagBits>& memoryProperty,
        const void* data = nullptr)
    {
        vk::Buffer buffer = device.createBuffer(vk::BufferCreateInfo({.size=size, .usage=usage, .sharingMode=vk::SharingMode::eExclusive}));
        
        vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateFlagsInfo allocateFlagsInfo = {.flags=vk::MemoryAllocateFlagBits::eDeviceAddress};

        vk::MemoryAllocateInfo allocateInfo = {.pNext=&allocateFlagsInfo,
                                                .allocationSize=memoryRequirements.size,
                                                .memoryTypeIndex=findMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                                memoryProperty)};
        
        vk::DeviceMemory memory = device.allocateMemory(allocateInfo);
        device.bindBufferMemory(buffer, memory, 0);

        if (data)
        {
            void *mappedMemory = device.mapMemory(memory, 0, size);
            memcpy(mappedMemory, data, size);
            device.unmapMemory(memory);
        }

        return VulkanBuffer{
            .buffer = buffer,
            .memory=memory,
            .address = device.getBufferAddress({.buffer=buffer})
        };
    };

    // BLAS

    const uint32_t numTriangles = 1;

    struct Vertex {
        float pos[3];
    };

    const std::vector<Vertex> vertices = {
        {{1.0f, 1.0f, 0.0f}},
        {{-1.0f, 1.0f, 0.0f}},
        {{0.0, -1.0f, 0.0f}}
    };

    std::vector<uint32_t> indices = {0, 1, 2};
    uint32_t indexCount = static_cast<uint32_t>(indices.size());

    const VkTransformMatrixKHR transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f
    };
    
    const vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    const vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | 
        vk::MemoryPropertyFlagBits::eDeviceLocal;

    VulkanBuffer vertexBuffer = createBuffer(vertices.size() * sizeof(Vertex), usageFlags, memoryFlags, vertices.data());
    VulkanBuffer indexBuffer = createBuffer(indices.size() * sizeof(uint32_t), usageFlags, memoryFlags, indices.data());
    VulkanBuffer transformBuffer = createBuffer(sizeof(VkTransformMatrixKHR), usageFlags, memoryFlags, &transformMatrix);

    vk::DeviceOrHostAddressConstKHR vertexBufferDeviceAddress{.deviceAddress=vertexBuffer.address};
    vk::DeviceOrHostAddressConstKHR indexBufferDeviceAddress{.deviceAddress=indexBuffer.address};
    vk::DeviceOrHostAddressConstKHR transformBufferDeviceAddress{.deviceAddress=transformBuffer.address};

    auto geometryBLAS = vk::AccelerationStructureGeometryKHR{
        .geometryType=vk::GeometryTypeKHR::eTriangles,
        .geometry=vk::AccelerationStructureGeometryDataKHR{
            vk::AccelerationStructureGeometryTrianglesDataKHR{
                .vertexFormat=vk::Format::eR32G32B32A32Sfloat,
                .vertexData=vertexBufferDeviceAddress,
                .vertexStride=sizeof(Vertex),
                .maxVertex=0,
                .indexType=vk::IndexType::eUint32,
                .indexData=indexBufferDeviceAddress,
                .transformData=transformBufferDeviceAddress,
            }
        },
        .flags=vk::GeometryFlagBitsKHR::eOpaque,
    };

    vk::AccelerationStructureBuildGeometryInfoKHR buildInfoBLAS = {
        .type=vk::AccelerationStructureTypeKHR::eBottomLevel,
        .flags=vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .mode=vk::BuildAccelerationStructureModeKHR::eBuild,
        .srcAccelerationStructure=nullptr,
        .dstAccelerationStructure=nullptr,
        .geometryCount=1,
        .pGeometries=&geometryBLAS,
        .scratchData={}
    };

    // get size info
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfoBLAS,
        numTriangles,
        dynamicDispatchLoader
    );

    // Keep track of all acceleration structure pieces, you can create a structure
    struct VulkanAccelerationStructure
    {
        vk::AccelerationStructureKHR accelerationStructure;
        VulkanBuffer structureBuffer;
        VulkanBuffer scratchBuffer;
        VulkanBuffer instancesBuffer;
    };

    VulkanAccelerationStructure bottomAccelerationStructure;
    // Allocate buffers for acceleration structure
    bottomAccelerationStructure.structureBuffer = createBuffer(buildSizesInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    bottomAccelerationStructure.scratchBuffer = createBuffer(buildSizesInfo.buildScratchSize, 
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // CREATE the acceleration structures
    vk::AccelerationStructureCreateInfoKHR createInfoBLAS
    {
        .buffer=bottomAccelerationStructure.structureBuffer.buffer,
        .offset = 0,
        .size=buildSizesInfo.accelerationStructureSize,
        .type=vk::AccelerationStructureTypeKHR::eBottomLevel
    };

    bottomAccelerationStructure.accelerationStructure = device.createAccelerationStructureKHR(createInfoBLAS, nullptr,
        dynamicDispatchLoader);
    
    // Fill remaining meta info
    buildInfoBLAS.dstAccelerationStructure=bottomAccelerationStructure.accelerationStructure;
    buildInfoBLAS.scratchData.deviceAddress=device.getBufferAddress({.buffer=bottomAccelerationStructure.scratchBuffer.buffer});

    // BUILD acceleration structure
    vk::AccelerationStructureBuildRangeInfoKHR buildRangeInfoBLAS = 
    {
        .primitiveCount=numTriangles,
        .primitiveOffset=0,
        .firstVertex=0,
        .transformOffset=0
    };

    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfoBLAS[] = {&buildRangeInfoBLAS};

    [&device, &commandPool, &computePresentQueue, &buildInfoBLAS, &pBuildRangeInfoBLAS, &dynamicDispatchLoader]()
    {
        vk::CommandBuffer singleTimeCommandBuffer = device.allocateCommandBuffers(
            {
                .commandPool=commandPool,
                .level=vk::CommandBufferLevel::ePrimary,
                .commandBufferCount=1              
            }
        ).front();

        vk::CommandBufferBeginInfo beginInfo = {
            .flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        VK_CHECK_RESULT(singleTimeCommandBuffer.begin(&beginInfo));

        singleTimeCommandBuffer.buildAccelerationStructuresKHR(1, &buildInfoBLAS, pBuildRangeInfoBLAS, dynamicDispatchLoader);
    
        singleTimeCommandBuffer.end();

        vk::SubmitInfo submitInfo = {
            .commandBufferCount=1,
            .pCommandBuffers=&singleTimeCommandBuffer
        };

        vk::Fence f = device.createFence({});
        VK_CHECK_RESULT(computePresentQueue.submit(1, &submitInfo, f));
        VK_CHECK_RESULT(device.waitForFences(1, &f, true, UINT64_MAX));

        device.destroyFence(f);
        device.freeCommandBuffers(commandPool, singleTimeCommandBuffer);
    }();

    //
    // TLAS - Top Level Acceleration Structure
    //
    // Acceleration structure meta info p.40
    auto geometryTLAS = vk::AccelerationStructureGeometryKHR{
        .geometryType=vk::GeometryTypeKHR::eInstances,
        .geometry=vk::AccelerationStructureGeometryDataKHR{
            .instances=vk::AccelerationStructureGeometryInstancesDataKHR
            {
                .arrayOfPointers=false
            }
        },
        .flags=vk::GeometryFlagBitsKHR::eOpaque
    };

    auto buildInfoTLAS = vk::AccelerationStructureBuildGeometryInfoKHR{
        .type=vk::AccelerationStructureTypeKHR::eTopLevel,
        .flags=vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .mode=vk::BuildAccelerationStructureModeKHR::eBuild,
        .srcAccelerationStructure=nullptr,
        .dstAccelerationStructure=nullptr,
        .geometryCount=1,
        .pGeometries=&geometryTLAS,
        .scratchData={}
    };

    // Calculate the required size for the acc structure
    auto buildSizesInfoTLAS = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfoTLAS, {1}, dynamicDispatchLoader
    );

    VulkanAccelerationStructure topAccelerationStructure;

    // Allocate buffer for the acceleration structure
    topAccelerationStructure.structureBuffer = createBuffer(buildSizesInfoTLAS.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    topAccelerationStructure.scratchBuffer = createBuffer(buildSizesInfoTLAS.buildScratchSize,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal);

    // Create allocation structure TLAS
    auto createInfoTLAS = vk::AccelerationStructureCreateInfoKHR{.buffer=topAccelerationStructure.structureBuffer.buffer,
                                                                .offset = 0,
                                                                .size=buildSizesInfoTLAS.accelerationStructureSize,
                                                                .type=vk::AccelerationStructureTypeKHR::eTopLevel};

    topAccelerationStructure.accelerationStructure = device.createAccelerationStructureKHR(createInfoTLAS, nullptr, dynamicDispatchLoader);
    // instance data tlas
    vk::TransformMatrixKHR vktransformMatrix;
    memcpy(&vktransformMatrix.matrix, &transformMatrix.matrix, sizeof(transformMatrix));

    auto accelerationStructureInstance = vk::AccelerationStructureInstanceKHR{
        .transform=vktransformMatrix,
        .instanceCustomIndex=0,
        .mask=0xFF,
        .instanceShaderBindingTableRecordOffset=0,
        .flags=VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR // ek::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable
    };

    accelerationStructureInstance.accelerationStructureReference=device.getAccelerationStructureAddressKHR({
        .accelerationStructure=bottomAccelerationStructure.accelerationStructure}, dynamicDispatchLoader);
    
    topAccelerationStructure.instancesBuffer = createBuffer(sizeof(vk::AccelerationStructureInstanceKHR),
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eHostVisible);

    void* pInstancesBuffer = device.mapMemory(topAccelerationStructure.instancesBuffer.memory, 0,
        sizeof(vk::AccelerationStructureInstanceKHR));
    memcpy(pInstancesBuffer, &accelerationStructureInstance, sizeof(vk::AccelerationStructureInstanceKHR));
    device.unmapMemory(topAccelerationStructure.instancesBuffer.memory);

    // fill in remaining TLAS info
    buildInfoTLAS.dstAccelerationStructure = topAccelerationStructure.accelerationStructure;
    buildInfoTLAS.scratchData.deviceAddress = device.getBufferAddress({.buffer=topAccelerationStructure.scratchBuffer.buffer});

    geometryTLAS.geometry.instances.data.deviceAddress = device.getBufferAddress({.buffer=topAccelerationStructure.instancesBuffer.buffer});
    // Build TLAS
    auto buildRangeInfoTLAS = vk::AccelerationStructureBuildRangeInfoKHR{
        .primitiveCount=1,
        .primitiveOffset=0,
        .firstVertex=0,
        .transformOffset=0,
    };

    const vk::AccelerationStructureBuildRangeInfoKHR* pBuildRangeInfoTLAS[] = {&buildRangeInfoTLAS};
    [&device, &commandPool, &computePresentQueue, &buildInfoTLAS, &pBuildRangeInfoTLAS, &dynamicDispatchLoader]()
    {
        // allocate command buffer for TLAS build
        vk::CommandBuffer singleTimeBuffer = device.allocateCommandBuffers(
            {
            .commandPool = commandPool,
            .level=vk::CommandBufferLevel::ePrimary,
            .commandBufferCount=1
            }
        ).front();

        vk::CommandBufferBeginInfo beginInfo = {
            .flags=vk::CommandBufferUsageFlagBits::eOneTimeSubmit
        };

        VK_CHECK_RESULT(singleTimeBuffer.begin(&beginInfo));
        singleTimeBuffer.buildAccelerationStructuresKHR(1, &buildInfoTLAS, pBuildRangeInfoTLAS, dynamicDispatchLoader);
        singleTimeBuffer.end();
        vk::SubmitInfo submitInfo=
        {
            .commandBufferCount=1,
            .pCommandBuffers=&singleTimeBuffer
        };

        vk::Fence f = device.createFence({});
        VK_CHECK_RESULT(computePresentQueue.submit(1, &submitInfo, f));
        VK_CHECK_RESULT(device.waitForFences(1, &f, true, UINT64_MAX));

        device.destroyFence(f);
        device.freeCommandBuffers(commandPool, singleTimeBuffer);
    }();

    struct 
    {
        uint32_t windowWidth;
        uint32_t windowHeight;
    } settings{.windowWidth=WINDOW_WIDTH, .windowHeight=WINDOW_HEIGHT};

    auto createImageView = [&device](const vk::Image& image, const vk::Format& format)
    {
        return device.createImageView(
            {
                .image=image,
                .viewType=vk::ImageViewType::e2D,
                .format=format,
                .subresourceRange=
                {
                    .aspectMask=vk::ImageAspectFlagBits::eColor,
                    .baseMipLevel=0,
                    .levelCount=1,
                    .baseArrayLayer=0,
                    .layerCount=1
                }
            }
        );
    };

    struct VulkanImage
    {
        vk::Image image;
        vk::DeviceMemory memory;
        vk::ImageView imageView;
    };

    auto createImage = [&createImageView, &findMemoryTypeIndex, &settings, &device, &physicalDevice] (const vk::Format& format,
        const vk::Flags<vk::ImageUsageFlagBits>& usageFlagBits)
    {
        vk::ImageCreateInfo imageCreateInfo = {
            .imageType=vk::ImageType::e2D,
            .format=format,
            .extent={.width=settings.windowWidth, .height=settings.windowHeight, .depth=1},
            .mipLevels=1,
            .arrayLayers=1,
            .samples=vk::SampleCountFlagBits::e1,
            .tiling=vk::ImageTiling::eOptimal,
            .usage=usageFlagBits,
            .sharingMode=vk::SharingMode::eExclusive,
            .initialLayout=vk::ImageLayout::eUndefined
        };

        vk::Image image = device.createImage(imageCreateInfo);

        vk::MemoryRequirements memoryRequirements = device.getImageMemoryRequirements(image);
        
        vk::MemoryAllocateInfo allocateInfo = 
        {
            .allocationSize=memoryRequirements.size,
            .memoryTypeIndex=findMemoryTypeIndex(memoryRequirements.memoryTypeBits,
                                                vk::MemoryPropertyFlagBits::eDeviceLocal)
        };

        vk::DeviceMemory memory = device.allocateMemory(allocateInfo);
        device.bindImageMemory(image, memory, 0);
        return VulkanImage{
            .image = image,
            .memory=memory,
            .imageView=createImageView(image, format)
        };
    };
    
    VkSurfaceKHR surface;
    SDL_Vulkan_CreateSurface(window, instance, &surface );

    const uint32_t imageCount=3;
    const vk::Format swapChainImageFormat=vk::Format::eB8G8R8A8Unorm; // vk::Format::eR8G8B8A8Unorm

    // Create Swap Chain
    vk::SwapchainKHR swapChain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
        .surface=surface,
        .minImageCount=imageCount,
        .imageFormat=swapChainImageFormat, // VK_FORMAT_B8G8R8A8_UNORM
        .imageColorSpace=vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent={.width=settings.windowWidth, .height=settings.windowHeight},
        .imageArrayLayers=1,
        .imageUsage=vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst,
        .imageSharingMode=vk::SharingMode::eExclusive,
        .preTransform=physicalDevice.getSurfaceCapabilitiesKHR(surface).currentTransform,
        .compositeAlpha=vk::CompositeAlphaFlagBitsKHR::eOpaque,
        .presentMode=vk::PresentModeKHR::eFifo, // vk::PresentModeKHR::eImmediate
        .clipped=true,
        .oldSwapchain=nullptr
    });

    // swap chain images
    vk::ImageView swapChainImageViews[imageCount];
    std::vector<vk::Image> swapChainImages=device.getSwapchainImagesKHR(swapChain);
    for(int nn = 0; nn < imageCount; nn++)
    {
        auto image = swapChainImages[nn];
        swapChainImageViews[nn] = createImageView(image, swapChainImageFormat);
    }

    // Create Images
    VulkanImage renderTargetImage = createImage(swapChainImageFormat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc);

    // Descriptors/Bindings
    vk::DescriptorSet rtDescriptorSet;
    vk::DescriptorSetLayout rtDescriptorSetLayout;
    VulkanBuffer uniformBuffer;

    [&device, &settings, &createBuffer, &renderTargetImage, &topAccelerationStructure, &rtDescriptorSet, &rtDescriptorSetLayout, &uniformBuffer]()
    {
        struct UniformData
        {
            glm::mat4 viewInverse;
            glm::mat4 projInverse;
        };
        UniformData uniformData{};
        uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(60.0f), (float)settings.windowWidth/(float)settings.windowHeight, 0.1f, 1000.0f));
        uniformData.viewInverse = glm::inverse(glm::lookAt(glm::vec3(0.0, 0.0, -2.5), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
        const vk::DeviceSize uniformBufferSize = sizeof(uniformData);

        uniformBuffer=createBuffer(uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal, &uniformData);

        // Create Descriptor Set Layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings = {
            {.binding=0, .descriptorType=vk::DescriptorType::eStorageImage, .descriptorCount=1, .stageFlags=vk::ShaderStageFlagBits::eRaygenKHR},
            {.binding=1, .descriptorType=vk::DescriptorType::eAccelerationStructureKHR, .descriptorCount=1, .stageFlags=vk::ShaderStageFlagBits::eRaygenKHR},
            {.binding=2, .descriptorType=vk::DescriptorType::eUniformBuffer, .descriptorCount=1, .stageFlags=vk::ShaderStageFlagBits::eRaygenKHR},
        };

        rtDescriptorSetLayout=device.createDescriptorSetLayout({.bindingCount=static_cast<uint32_t>(bindings.size()), .pBindings=bindings.data()});

        // Create Descriptor Pool
        std::vector<vk::DescriptorPoolSize> poolSizes={
            {.type=vk::DescriptorType::eStorageImage,                   .descriptorCount=1},
            {.type=vk::DescriptorType::eAccelerationStructureKHR,       .descriptorCount=1},
            {.type=vk::DescriptorType::eUniformBuffer,                  .descriptorCount=1},
        };

        vk::DescriptorPool rtDescriptorPool = device.createDescriptorPool(
        {
            .maxSets=1,
            .poolSizeCount=static_cast<uint32_t>(poolSizes.size()),
            .pPoolSizes=poolSizes.data()
        });

        // Create Descriptor Set
        rtDescriptorSet = device.allocateDescriptorSets( // vk::DescriptorSet
        {
            .descriptorPool=rtDescriptorPool,
            .descriptorSetCount=1,
            .pSetLayouts=&rtDescriptorSetLayout
        }).front();

        auto renderTargetImageInfo = vk::DescriptorImageInfo{
            .imageView=renderTargetImage.imageView,
            .imageLayout=vk::ImageLayout::eGeneral
        };

        auto accelerationStructureInfo = vk::WriteDescriptorSetAccelerationStructureKHR{
            .accelerationStructureCount=1,
            .pAccelerationStructures=&topAccelerationStructure.accelerationStructure
        };

        auto uniformBufferInfo=vk::DescriptorBufferInfo{
            .buffer=uniformBuffer.buffer,
            .offset=0,
            .range=uniformBufferSize
        };

        std::vector<vk::WriteDescriptorSet> descriptorWrites = {
            {.dstSet=rtDescriptorSet, .dstBinding=0, .dstArrayElement=0, .descriptorCount=1, 
             .descriptorType=vk::DescriptorType::eStorageImage, .pImageInfo=&renderTargetImageInfo},

            {.pNext=&accelerationStructureInfo, 
             .dstSet=rtDescriptorSet, .dstBinding=1, .dstArrayElement=0, .descriptorCount=1, 
             .descriptorType=vk::DescriptorType::eAccelerationStructureKHR},

            {.dstSet=rtDescriptorSet, .dstBinding=2, .dstArrayElement=0, .descriptorCount=1, 
             .descriptorType=vk::DescriptorType::eUniformBuffer, .pBufferInfo=&uniformBufferInfo},
        };

        device.updateDescriptorSets(static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }();

    // Shaders
    auto createShaderModule = [&device] (const std::string& path)
    {
        auto readBinaryFile = [](const std::string& path)
        {
            std::ifstream file(path, std::ios::ate | std::ios::binary);
            DBG_ASSERT_WARN(file.is_open(), "[Error] Failed to open file at '" + path + "'!");

            size_t fileSize = (size_t)file.tellg();
            std::vector<char> buffer(fileSize);

            file.seekg(0);
            file.read(buffer.data(), fileSize);
            file.close();
            return buffer;
        };

        std::vector<char> shaderCode = readBinaryFile(path);

        vk::ShaderModuleCreateInfo shaderModuleCreateInfo = {
            .codeSize=shaderCode.size(),
            .pCode=reinterpret_cast<const uint32_t*>(shaderCode.data())
        };

        return device.createShaderModule(shaderModuleCreateInfo);
    };

    // auto createShaderModuleFromGLSL = [&device] (const std::string& glslSourceCode, shaderc_shader_kind shaderKind)
    // {
    //     const char* shaderSource = glslSourceCode.c_str();
        
    //     shaderc::Compiler compiler;
    //     shaderc::CompileOptions options;
    //     options.SetTargetSpirv(shaderc_spirv_version_1_6);

    //     shaderc::SpvCompilationResult module = compiler.CompileGlslToSpv(shaderSource,
    //         strlen(shaderSource),
    //         shaderKind,
    //         "shader.rmiss.spv",
    //         options);
    //     if (module.GetCompilationStatus() != shaderc_compilation_status_success)
    //     {
    //         std::cerr << module.GetErrorMessage() << std::endl;
    //         DBG_ASSERT(0);
    //     }

    //     // retrieve spir v byte code from compilation result
    //     const auto spirvCode = module.cbegin();
    //     const size_t spirvSize = (size_t)(module.cend() - module.cbegin()) * 4; // uint32 to char

    //     // create vulkan shader module
    //     vk::ShaderModuleCreateInfo createInfo{.codeSize=spirvSize, .pCode=spirvCode};
    //     return device.createShaderModule(createInfo);
    // };
    
    auto createShaderModuleFromPreCompiledSPIRV = [&device] (const std::string& path)
    {
        std::ifstream rayMissFile(path, std::ios::binary | std::ios::ate);
        if (rayMissFile.fail()) LOG("Failed miss shader");
        std::streamsize rayMissFileSize = rayMissFile.tellg();
        rayMissFile.seekg(0, std::ios::beg);
        std::vector<uint32_t> rayMissShaderSource(rayMissFileSize / sizeof(uint32_t));

        rayMissFile.read(reinterpret_cast<char *>(rayMissShaderSource.data()),
                        rayMissFileSize);

        rayMissFile.close();

        // VkShaderModuleCreateInfo rayMissShaderModuleCreateInfo = {
        //     .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        //     .pNext = NULL,
        //     .flags = 0,
        //     .codeSize = (uint32_t)rayMissShaderSource.size() * sizeof(uint32_t),
        //     .pCode = rayMissShaderSource.data()};

        // VkShaderModule rayMissShaderModuleHandle = VK_NULL_HANDLE;

        vk::ShaderModuleCreateInfo createInfo{.codeSize=(uint32_t)rayMissShaderSource.size() * sizeof(uint32_t), .pCode=rayMissShaderSource.data()};
        return device.createShaderModule(createInfo);
    };
    // Create shader modules from inline
    // vk::ShaderModule raygenModule = createShaderModuleFromGLSL(raygenShaderCode, shaderc_shader_kind::shaderc_raygen_shader); 
    // vk::ShaderModule chitModule = createShaderModuleFromGLSL(closestHitShaderCode, shaderc_shader_kind::shaderc_closesthit_shader);
    // vk::ShaderModule missModule = createShaderModuleFromGLSL(missShaderCode, shaderc_shader_kind::shaderc_miss_shader);

    // Using pre-compiled SPIRV shaders

    vk::ShaderModule raygenModule = createShaderModuleFromPreCompiledSPIRV("compiled_shaders/shader.rgen.spv");
    vk::ShaderModule chitModule = createShaderModuleFromPreCompiledSPIRV("compiled_shaders/shader.rchit.spv");
    vk::ShaderModule missModule = createShaderModuleFromPreCompiledSPIRV("compiled_shaders/shader.rmiss.spv");

    std::vector<vk::PipelineShaderStageCreateInfo> stages = {
        {.stage=vk::ShaderStageFlagBits::eRaygenKHR,                .module=raygenModule,       .pName="main"},
        {.stage=vk::ShaderStageFlagBits::eMissKHR,                  .module=missModule,         .pName="main"},
        {.stage=vk::ShaderStageFlagBits::eClosestHitKHR,            .module=chitModule,         .pName="main"},
    };

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups = 
    {
        {.type=vk::RayTracingShaderGroupTypeKHR::eGeneral,                  .generalShader=0,
        .closestHitShader=VK_SHADER_UNUSED_KHR, .anyHitShader=VK_SHADER_UNUSED_KHR, .intersectionShader=VK_SHADER_UNUSED_KHR},
        {.type=vk::RayTracingShaderGroupTypeKHR::eGeneral,                  .generalShader=1,
        .closestHitShader=VK_SHADER_UNUSED_KHR, .anyHitShader=VK_SHADER_UNUSED_KHR, .intersectionShader=VK_SHADER_UNUSED_KHR},
        {.type=vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,      .generalShader=VK_SHADER_UNUSED_KHR,
        .closestHitShader=2, .anyHitShader=VK_SHADER_UNUSED_KHR, .intersectionShader=VK_SHADER_UNUSED_KHR}
    };

    vk::PipelineLayout rtPipelineLayout = device.createPipelineLayout( //vk::PipelineLayout
    {
        .setLayoutCount=1,
        .pSetLayouts=&rtDescriptorSetLayout,
        .pushConstantRangeCount=0,
        .pPushConstantRanges=nullptr
    });

    vk::PipelineLibraryCreateInfoKHR libraryCreateInfo = {.libraryCount=0};
    vk::RayTracingPipelineCreateInfoKHR pipelineCreateInfo = {
        .stageCount=static_cast<uint32_t>(stages.size()),
        .pStages=stages.data(),
        .groupCount=static_cast<uint32_t>(groups.size()),
        .pGroups=groups.data(),
        .maxPipelineRayRecursionDepth=getRayTracingProperties(physicalDevice).maxRayRecursionDepth,
        .pLibraryInfo=&libraryCreateInfo,
        .pLibraryInterface=nullptr,
        .layout=rtPipelineLayout,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=0
    };

    vk::Pipeline rtPipeline = device.createRayTracingPipelineKHR(nullptr, nullptr, pipelineCreateInfo, nullptr, dynamicDispatchLoader).value;

    // clean up shader modules after pipeline creation
    device.destroyShaderModule(raygenModule);
    device.destroyShaderModule(chitModule);
    device.destroyShaderModule(missModule);

    // Create Shader Binding Table
    vk::StridedDeviceAddressRegionKHR sbtRayGenAddressRegion;
    vk::StridedDeviceAddressRegionKHR sbtMissAddressingRegion;
    vk::StridedDeviceAddressRegionKHR sbtHitAddressRegion;
    
    [&device, &createBuffer, &rtPipeline, &getRayTracingProperties, &dynamicDispatchLoader, &physicalDevice] (
        vk::StridedDeviceAddressRegionKHR& sbtRayGenAddressRegion,
        vk::StridedDeviceAddressRegionKHR& sbtMissAddressRegion,
        vk::StridedDeviceAddressRegionKHR& sbtHitAddressRegion)
    {
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = getRayTracingProperties(physicalDevice);
        uint32_t baseAlignment = rayTracingProperties.shaderGroupBaseAlignment;
        uint32_t handleSize = rayTracingProperties.shaderGroupHandleSize;

        const uint32_t shaderGroupCount = 3;
        vk::DeviceSize sbtBufferSize = baseAlignment * shaderGroupCount;

        VulkanBuffer shaderBindingTableBuffer = createBuffer(sbtBufferSize,
        vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal);

        std::vector<uint8_t> handles = device.getRayTracingShaderGroupHandlesKHR<uint8_t>(rtPipeline, 0, shaderGroupCount, 
            shaderGroupCount * handleSize,
            dynamicDispatchLoader);
        
        vk::DeviceAddress sbtAddress = device.getBufferAddress({.buffer=shaderBindingTableBuffer.buffer});

        sbtRayGenAddressRegion = vk::StridedDeviceAddressRegionKHR(
            {.deviceAddress=sbtAddress + baseAlignment * 0, .stride=baseAlignment, .size=baseAlignment});
        sbtMissAddressRegion = vk::StridedDeviceAddressRegionKHR(
            {.deviceAddress=sbtAddress + baseAlignment * 1, .stride=baseAlignment, .size=baseAlignment});
        sbtHitAddressRegion = vk::StridedDeviceAddressRegionKHR(
            {.deviceAddress=sbtAddress + baseAlignment * 2, .stride=baseAlignment, .size=baseAlignment});

        uint8_t* sbtBufferData = static_cast<uint8_t*>(device.mapMemory(shaderBindingTableBuffer.memory, 0, sbtBufferSize));
        memcpy(sbtBufferData, handles.data(), handleSize);
        memcpy(sbtBufferData + baseAlignment, handles.data() + handleSize, handleSize);
        memcpy(sbtBufferData + baseAlignment * 2, handles.data() + handleSize * 2, handleSize);
        device.unmapMemory(shaderBindingTableBuffer.memory);
    }(sbtRayGenAddressRegion, sbtMissAddressingRegion, sbtHitAddressRegion);

    // Ray tracing Output
    auto getImagePipelineBarrier = [](const vk::AccessFlagBits& srcAccessFlags, const vk::AccessFlagBits& dstAccessFlags,
        const vk::ImageLayout& oldLayout, const vk::ImageLayout& newLayout,
        const vk::Image& image, uint32_t computeQueueFamily)
    {
        return vk::ImageMemoryBarrier{
            .srcAccessMask=srcAccessFlags,
            .dstAccessMask=dstAccessFlags,
            .oldLayout=oldLayout,
            .newLayout=newLayout,
            .srcQueueFamilyIndex=computeQueueFamily,
            .dstQueueFamilyIndex=computeQueueFamily,
            .image=image,
            .subresourceRange={
                .aspectMask=vk::ImageAspectFlagBits::eColor,
                .baseMipLevel=0,
                .levelCount=1,
                .baseArrayLayer=0,
                .layerCount=1
            },
        };
    };

    std::vector<vk::CommandBuffer> commandBuffers = device.allocateCommandBuffers(
        {
            .commandPool=commandPool,
            .level=vk::CommandBufferLevel::ePrimary,
            .commandBufferCount=imageCount
        }
    );

    #if 1
    for(size_t nn = 0; nn < commandBuffers.size(); nn++)
    {
        vk::CommandBufferBeginInfo beginInfo = {};
        VK_CHECK_RESULT(commandBuffers[nn].begin(&beginInfo));

        // Render target image & summed pixel color image: Undefined -> General
        vk::ImageMemoryBarrier imageBarriersToGeneral[2] = {
            getImagePipelineBarrier(
                vk::AccessFlagBits::eNoneKHR, vk::AccessFlagBits::eShaderWrite,
                vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral, renderTargetImage.image, queueId
            )
        };

        commandBuffers[nn].pipelineBarrier(vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            vk::PipelineStageFlagBits::eRayTracingShaderKHR,
            vk::DependencyFlagBits::eByRegion, 0, nullptr,
            0, nullptr, 1, imageBarriersToGeneral);

        // Ray Tracing
        commandBuffers[nn].bindPipeline(vk::PipelineBindPoint::eRayTracingKHR, rtPipeline);
        
        std::vector<vk::DescriptorSet> descriptorSets={rtDescriptorSet};
        commandBuffers[nn].bindDescriptorSets(vk::PipelineBindPoint::eRayTracingKHR, rtPipelineLayout,
            0, descriptorSets, nullptr);
        
        commandBuffers[nn].traceRaysKHR(sbtRayGenAddressRegion, sbtMissAddressingRegion, sbtHitAddressRegion, {},
            settings.windowWidth, settings.windowHeight, 1, dynamicDispatchLoader);

        // Render target image: General -> Transfer src & swap chain image : undefined => transfor dst
        vk::ImageMemoryBarrier imageBarriersToTransfer[2] = 
        {
            getImagePipelineBarrier(
                vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eTransferRead,
                vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral, renderTargetImage.image, queueId
            ),
            getImagePipelineBarrier(
                vk::AccessFlagBits::eNoneKHR, vk::AccessFlagBits::eTransferWrite, 
                vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, swapChainImages[nn], queueId
            )
        };

        commandBuffers[nn].pipelineBarrier(vk::PipelineStageFlagBits::eRayTracingShaderKHR, vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::eByRegion, 0, nullptr, 0, nullptr, 2, imageBarriersToTransfer);

        // Copy render target image to swap chainimage
        vk::ImageSubresourceLayers subresourceLayers = {
            .aspectMask=vk::ImageAspectFlagBits::eColor,
            .mipLevel=0,
            .baseArrayLayer=0,
            .layerCount=1
        };

        vk::ImageCopy imageCopy = {
            .srcSubresource=subresourceLayers, .srcOffset={0,0,0},
            .dstSubresource=subresourceLayers, .dstOffset={0,0,0},
            .extent={.width=settings.windowWidth, .height=settings.windowHeight, .depth=1}
        };

        commandBuffers[nn].copyImage(renderTargetImage.image, vk::ImageLayout::eGeneral, swapChainImages[nn], vk::ImageLayout::eTransferDstOptimal, 1, &imageCopy);

        // Swap chain images - transfer -> present
        vk::ImageMemoryBarrier barrierSwapChainToPresent = getImagePipelineBarrier(
            vk::AccessFlagBits::eTransferWrite, vk::AccessFlagBits::eMemoryRead,
            vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::ePresentSrcKHR, swapChainImages[nn], queueId
        );

        commandBuffers[nn].pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::eByRegion, 0, nullptr,
            0, nullptr, 1, &barrierSwapChainToPresent);
        
        commandBuffers[nn].end();
    }
    #endif

    vk::Fence fence = device.createFence({});

    vk::Semaphore semaphore = device.createSemaphore({});
    vk::Semaphore semaphore2 = device.createSemaphore({});

    // ------------------Render Loop-------------------

    float yAngle = 0;
    bool running = true;
    while (running) {
        SDL_Event windowEvent;
        while(SDL_PollEvent(&windowEvent))
        {
            if(windowEvent.type == SDL_QUIT) {
                running = false;
                break;
            }
        }
    
        struct UniformData
        {
            glm::mat4 viewInverse;
            glm::mat4 projInverse;
        };

        auto updateUniformBuffer = [&device, &uniformBuffer](UniformData& uniformData)
        {
            void* data = device.mapMemory(uniformBuffer.memory, 0, sizeof(uniformBuffer));
            memcpy(data, &uniformData, sizeof(uniformData));
            device.unmapMemory(uniformBuffer.memory);
        };

        float dist = 2.5f;
        yAngle += 0.05f;
        glm::mat4 ident(1.0f);
        glm::mat4 rotY = glm::rotate(ident, yAngle, glm::vec3(0.0, 1.0f, 0.0f));
        glm::vec3 camZ = glm::vec3(rotY[0][0] * dist, rotY[0][1] * dist, rotY[0][2] * dist);

        UniformData uniformData{};
        uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(60.0f), (float)settings.windowWidth / (float)settings.windowHeight, 0.1f, 1000.0f));
        uniformData.viewInverse = glm::inverse(glm::lookAt(camZ, glm::vec3(0,0,0), glm::vec3(0, 1, 0)));
        updateUniformBuffer(uniformData);

        auto swapChainImageIndex = device.acquireNextImageKHR(swapChain, std::numeric_limits<uint64_t>::max(), semaphore2, {}).value;

        vk::PipelineStageFlags waitStageMask = vk::PipelineStageFlagBits::eTransfer;

        device.resetFences(fence);

        vk::SubmitInfo submitInfo = {
            .waitSemaphoreCount=1,
            .pWaitSemaphores=&semaphore2,
            .pWaitDstStageMask=&waitStageMask,
            .commandBufferCount=1,
            .pCommandBuffers=&commandBuffers[swapChainImageIndex],
            .signalSemaphoreCount=1,
            .pSignalSemaphores=&semaphore
        };

        VK_CHECK_RESULT(computePresentQueue.submit(1, &submitInfo, fence));

        VK_CHECK_RESULT(device.waitForFences(1, &fence, true, UINT64_MAX));
        device.resetFences(fence);

        vk::PresentInfoKHR presentInfo = {
            .waitSemaphoreCount=1,
            .pWaitSemaphores=&semaphore,
            .swapchainCount=1,
            .pSwapchains=&swapChain,
            .pImageIndices=&swapChainImageIndex
        };

        VK_CHECK_RESULT(computePresentQueue.presentKHR(presentInfo));

        device.waitIdle();
    }

    // Cleanup
    device.destroySemaphore(semaphore);
    device.destroySemaphore(semaphore2);
    device.destroyFence(fence);
    device.destroyPipeline(rtPipeline);
    device.destroyPipelineLayout(rtPipelineLayout);
    // device.destroyDescriptorSetLayout(rtDescriptorSetLayout);
    // device.destroyDescriptorPool(rtDescriptorPool);

    auto destroyBuffer = [&device](const VulkanBuffer& buffer)
    {
        device.destroyBuffer(buffer.buffer);
        device.freeMemory(buffer.memory);
    };

    auto destroyAccelerationStructure = [&device, &destroyBuffer](const VulkanAccelerationStructure& accelerationStructure, vk::DispatchLoaderDynamic& dynamicDispatchLoader)
    {
        device.destroyAccelerationStructureKHR(accelerationStructure.accelerationStructure, nullptr, dynamicDispatchLoader);
        destroyBuffer(accelerationStructure.structureBuffer);
        destroyBuffer(accelerationStructure.scratchBuffer);
        destroyBuffer(accelerationStructure.instancesBuffer);
    };

    destroyAccelerationStructure(topAccelerationStructure, dynamicDispatchLoader);
    destroyAccelerationStructure(bottomAccelerationStructure, dynamicDispatchLoader);

    // destroyBuffer(uniformBuffer, device);
    // destroyBuffer(shaderBindingTableBuffer, device);

    // device.destroyImageView(swapChainImageView); // todo
    device.destroySwapchainKHR(swapChain);
    device.destroyCommandPool(commandPool);

    auto destroyImage = [&device](const VulkanImage& image)
    {
        device.destroyImageView(image.imageView);
        device.destroyImage(image.image);
        device.freeMemory(image.memory);
    };

    destroyImage(renderTargetImage);

    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroy();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
#endif