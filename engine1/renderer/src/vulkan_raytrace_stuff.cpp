#include "vulkan_raytrace_stuff.hpp"
#include <iostream>
#include <set>
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <fstream>
#include <exception>
#include <string>

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
#pragma endregion

#pragma region vulkan raytrace stuff

#pragma region helpers declaration
///
/// helpers
std::vector<const char *> getExtensions(SDL_Window* window);
vk::PhysicalDevice getPhysicalDevice(vk::Instance& instance);
uint32_t getQueueId(vk::PhysicalDevice& physicalDevice);
vk::Device createLogicalDevice(uint32_t queueId, vk::PhysicalDevice& physicalDevice);
uint32_t findMemoryTypeIndex(vk::PhysicalDevice& physicalDevice, const uint32_t& memoryTypeBits, const vk::MemoryPropertyFlags& properties);
VulkanBuffer createBuffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, const vk::DeviceSize& size,
        const vk::Flags<vk::BufferUsageFlagBits>& usage,
        const vk::Flags<vk::MemoryPropertyFlagBits>& memoryProperty,
        const void* data);
void BuildRTAS(Mesh& mesh, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::DispatchLoaderDynamic& dynamicDispatchLoader, vk::CommandPool& commandPool, 
                vk::Queue& computePresentQueue, VulkanAccelerationStructure& topAccelerationStructure, VulkanAccelerationStructure& bottomAccelerationStructure);

vk::ImageView createImageView (vk::Device& device, const vk::Image& image, const vk::Format& format);
VulkanImage createImage(vk::Device& device, vk::PhysicalDevice& physicalDevice, const vk::Format& format,
        const vk::Flags<vk::ImageUsageFlagBits>& usageFlagBits, const uint32_t width, const uint32_t height);
void createDescriptor(vk::PhysicalDevice& physicalDevice, vk::Device& device, uint32_t width, uint32_t height, VulkanImage& renderTargetImage, 
                        VulkanAccelerationStructure& topAccelerationStructure, vk::DescriptorSet& rtDescriptorSet,
                        vk::DescriptorSetLayout& rtDescriptorSetLayout, VulkanBuffer& uniformBuffer);
vk::ShaderModule createShaderModuleFromPreCompiledSPIRV(vk::Device& device, const std::string& path);
void createShaderBindingTable(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Pipeline& rtPipeline, vk::DispatchLoaderDynamic& dynamicDispatchLoader, 
                            vk::StridedDeviceAddressRegionKHR& sbtRayGenAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtMissAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtHitAddressRegion);
void prepareCommandBuffers(std::vector<vk::CommandBuffer>& commandBuffers, VulkanImage& renderTargetImage, uint32_t queueId, vk::Pipeline& rtPipeline,
                            vk::DescriptorSet& rtDescriptorSet, vk::PipelineLayout& rtPipelineLayout,
                            vk::StridedDeviceAddressRegionKHR& sbtRayGenAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtMissAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtHitAddressRegion,
                            const uint32_t width, const uint32_t height, vk::DispatchLoaderDynamic& dynamicDispatchLoader, std::vector<vk::Image>& swapChainImages);
void updateUniformBuffer(vk::Device& device, VulkanBuffer& uniformBuffer, UniformData& uniformData);
void destroyBuffer(vk::Device& device, const VulkanBuffer& buffer);
void destroyAccelerationStructure (vk::Device& device, const VulkanAccelerationStructure& accelerationStructure, vk::DispatchLoaderDynamic& dynamicDispatchLoader);
void destroyImage(vk::Device& device, const VulkanImage& image);

#pragma endregion

#pragma region VulkanRaytraceStuff implementation

void VulkanRaytraceStuff::init(const char* appname, int width, int height)
{
    extent = vk::Extent2D({
        .width=static_cast<uint32_t>(width),.height=static_cast<uint32_t>(height)
    });

    // Init vulkan app info
    vk::ApplicationInfo appInfo;
    appInfo.setPApplicationName(appname)
            .setApplicationVersion(VK_MAKE_VERSION(1, 0, 0))
            .setPEngineName("No Engine")
            .setEngineVersion(VK_MAKE_VERSION(1, 0, 0))
            .setApiVersion(VK_API_VERSION_1_2);
    
    // Vulkan instance create info
    vk::InstanceCreateInfo instanceCreateInfo;
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window = SDL_CreateWindow(appname, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);
    std::vector<const char *> requiredInstanceExtensions = getExtensions(window);
    instanceCreateInfo.setPApplicationInfo(&appInfo);
    instanceCreateInfo.enabledExtensionCount = (uint32_t)requiredInstanceExtensions.size();
    instanceCreateInfo.ppEnabledExtensionNames = requiredInstanceExtensions.data();
    instance = (vk::createInstance(instanceCreateInfo));
    physicalDevice = (getPhysicalDevice(instance));
    queueId = (getQueueId(physicalDevice));
    std::cout << queueId << std::endl;
    device = (createLogicalDevice(queueId, physicalDevice));
    dynamicDispatchLoader = vk::DispatchLoaderDynamic(instance, vkGetInstanceProcAddr, device);
    computePresentQueue = device.getQueue(queueId, 0);
    commandPool = device.createCommandPool({.queueFamilyIndex=queueId});
    
    #pragma region RTAS
    Mesh mesh;    
    mesh.vertices = {
        {{0, 0.5f, 0.0f}},
        {{0.5f, 0.5f, 0.0f}},
        {{0.5f, 0, 0.0f}},
        {{0.0, 0, 0.0f}},
    };
    mesh.indices = {0, 1, 2, 0, 2, 3};

    mesh.transformMatrix = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
    };

    mesh.Prepare(physicalDevice, device);

    // rayTracingAccelerationStructure = RayTracingAccelerationStructure();
    // rayTracingAccelerationStructure.AddMesh(mesh, physicalDevice, device, dynamicDispatchLoader, commandPool, computePresentQueue);
    BuildRTAS(mesh, physicalDevice, device, dynamicDispatchLoader, commandPool, computePresentQueue, topAccelerationStructure, bottomAccelerationStructure);
    #pragma endregion

    SDL_Vulkan_CreateSurface(window, instance, &surface );

    const uint32_t imageCount=3;
    const vk::Format swapChainImageFormat=vk::Format::eB8G8R8A8Unorm; // vk::Format::eR8G8B8A8Unorm

    // Create Swap Chain
    swapChain = device.createSwapchainKHR(vk::SwapchainCreateInfoKHR{
        .surface=surface,
        .minImageCount=imageCount,
        .imageFormat=swapChainImageFormat, // VK_FORMAT_B8G8R8A8_UNORM
        .imageColorSpace=vk::ColorSpaceKHR::eSrgbNonlinear,
        .imageExtent=extent,
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
        swapChainImageViews[nn] = createImageView(device, image, swapChainImageFormat);
    }

    // Create Images
    renderTargetImage = createImage(device, physicalDevice, swapChainImageFormat, vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc, 
                                                extent.width, extent.height);

    // Descriptors/Bindings
    vk::DescriptorSet rtDescriptorSet;
    vk::DescriptorSetLayout rtDescriptorSetLayout;

    createDescriptor(physicalDevice, device, extent.width, extent.height, renderTargetImage, topAccelerationStructure, rtDescriptorSet, rtDescriptorSetLayout, cameraUniformBuffer);


    pipeline = std::make_unique<RayTracingPipeline>(device);
    pipeline->AddShader("compiled_shaders/shader.rgen.spv", Shader::ShaderType::RayGen);
    pipeline->AddShader("compiled_shaders/shader.rmiss.spv", Shader::ShaderType::Miss);
    pipeline->AddShader("compiled_shaders/shader.rchit.spv", Shader::ShaderType::ClosestHit);
    pipeline->CreatePipeline(physicalDevice, rtDescriptorSetLayout, dynamicDispatchLoader, getRayTracingProperties(physicalDevice).maxRayRecursionDepth);

    // Create Shader Binding Table
    vk::StridedDeviceAddressRegionKHR sbtRayGenAddressRegion;
    vk::StridedDeviceAddressRegionKHR sbtMissAddressRegion;
    vk::StridedDeviceAddressRegionKHR sbtHitAddressRegion;

    createShaderBindingTable(device, physicalDevice, pipeline->pipeline, dynamicDispatchLoader, sbtRayGenAddressRegion, sbtMissAddressRegion, sbtHitAddressRegion);

    commandBuffers = device.allocateCommandBuffers(
    {
        .commandPool=commandPool,
        .level=vk::CommandBufferLevel::ePrimary,
        .commandBufferCount=imageCount
    });

    prepareCommandBuffers(commandBuffers, renderTargetImage, queueId, pipeline->pipeline, rtDescriptorSet, pipeline->pipelineLayout, sbtRayGenAddressRegion, 
                        sbtMissAddressRegion, sbtHitAddressRegion, extent.width, extent.height, dynamicDispatchLoader, swapChainImages);

    fence = device.createFence({});

    semaphore = device.createSemaphore({});
    semaphore2 = device.createSemaphore({});


}

VulkanRaytraceStuff::~VulkanRaytraceStuff()
{
    LOG("Vulkan Raytracing Stuff (Destroy)");

    // Cleanup
    device.destroySemaphore(semaphore);
    device.destroySemaphore(semaphore2);
    device.destroyFence(fence);

    //** Moved to new class *//
    // device.destroyPipeline(rtPipeline);
    // device.destroyPipelineLayout(rtPipelineLayout);
    pipeline->DestroyPipeline();
    //                    //

    // device.destroyDescriptorSetLayout(rtDescriptorSetLayout);
    // device.destroyDescriptorPool(rtDescriptorPool);

    //** moved to new class */
    destroyAccelerationStructure(device, topAccelerationStructure, dynamicDispatchLoader);
    destroyAccelerationStructure(device, bottomAccelerationStructure, dynamicDispatchLoader);
    // rayTracingAccelerationStructure.Destroy(device, dynamicDispatchLoader);

    destroyBuffer(device, cameraUniformBuffer);
    // destroyBuffer(shaderBindingTableBuffer, device);

    // device.destroyImageView(swapChainImageView); // todo
    device.destroySwapchainKHR(swapChain);
    device.destroyCommandPool(commandPool);

    destroyImage(device, renderTargetImage);

    device.destroy();
    instance.destroySurfaceKHR(surface);
    instance.destroy();

    LOG("Vulkan Raytracing Stuff (Destroy) 2");
}

void VulkanRaytraceStuff::run()
{
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

        float dist = 2.5f;
        yAngle += 0.05f;
        glm::mat4 ident(1.0f);
        glm::mat4 rotY = glm::rotate(ident, yAngle, glm::vec3(0.0, 1.0f, 0.0f));
        glm::vec3 camZ = glm::vec3(rotY[0][0] * dist, rotY[0][1] * dist, rotY[0][2] * dist);

        UniformData uniformData{};
        uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(60.0f), (float)extent.width / (float)extent.height, 0.1f, 1000.0f));
        uniformData.viewInverse = glm::inverse(glm::lookAt(camZ, glm::vec3(0,0,0), glm::vec3(0, 1, 0)));
        updateUniformBuffer(device, cameraUniformBuffer, uniformData); // Send update to shader (materialInstance.SetUniform(uniformData))

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
}
#pragma endregion

#pragma region helpers implementation
std::vector<const char *> getExtensions(SDL_Window* window)
{
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
    return requiredInstanceExtensions;
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

uint32_t findMemoryTypeIndex(vk::PhysicalDevice& physicalDevice, const uint32_t& memoryTypeBits, const vk::MemoryPropertyFlags& properties)
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
}

VulkanBuffer createBuffer(vk::PhysicalDevice& physicalDevice, vk::Device& device, const vk::DeviceSize& size,
        const vk::Flags<vk::BufferUsageFlagBits>& usage,
        const vk::Flags<vk::MemoryPropertyFlagBits>& memoryProperty,
        const void* data = nullptr)
{
    vk::Buffer buffer = device.createBuffer(vk::BufferCreateInfo({.size=size, .usage=usage, .sharingMode=vk::SharingMode::eExclusive}));
        
        vk::MemoryRequirements memoryRequirements = device.getBufferMemoryRequirements(buffer);

        vk::MemoryAllocateFlagsInfo allocateFlagsInfo = {.flags=vk::MemoryAllocateFlagBits::eDeviceAddress};

        vk::MemoryAllocateInfo allocateInfo = {.pNext=&allocateFlagsInfo,
                                                .allocationSize=memoryRequirements.size,
                                                .memoryTypeIndex=findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits,
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
}


/// @brief Step 4
void BuildRTAS(Mesh& mesh, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::DispatchLoaderDynamic& dynamicDispatchLoader, vk::CommandPool& commandPool, 
vk::Queue& computePresentQueue, VulkanAccelerationStructure& topAccelerationStructure, VulkanAccelerationStructure& bottomAccelerationStructure)
{
    std::vector<vk::AccelerationStructureGeometryKHR> geometries;

    geometries.push_back(mesh.geometryBLAS);


    // TODO: Maybe here is where more mesh instances go
    vk::AccelerationStructureBuildGeometryInfoKHR buildInfoBLAS = {
        .type=vk::AccelerationStructureTypeKHR::eBottomLevel,
        .flags=vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace,
        .mode=vk::BuildAccelerationStructureModeKHR::eBuild,
        .srcAccelerationStructure=nullptr,
        .dstAccelerationStructure=nullptr,
        .geometryCount=static_cast<uint32_t>(geometries.size()), // # of AccelerationStructureGeometryKHR
        .pGeometries=geometries.data(),
        .scratchData={}
    };

    // get size info
    vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo = device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice,
        buildInfoBLAS,
        mesh.NumTriangles(),
        dynamicDispatchLoader
    );

    // Allocate buffers for acceleration structure
    bottomAccelerationStructure.structureBuffer = createBuffer(physicalDevice, device, buildSizesInfo.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
        vk::MemoryPropertyFlagBits::eDeviceLocal);
    bottomAccelerationStructure.scratchBuffer = createBuffer(physicalDevice, device, buildSizesInfo.buildScratchSize, 
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
        .primitiveCount=mesh.NumTriangles(),
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

    //VulkanRaytraceStuff::VulkanAccelerationStructure topAccelerationStructure;

    // Allocate buffer for the acceleration structure
    topAccelerationStructure.structureBuffer = createBuffer(physicalDevice, device, buildSizesInfoTLAS.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR,
        vk::MemoryPropertyFlagBits::eDeviceLocal
    );

    topAccelerationStructure.scratchBuffer = createBuffer(physicalDevice, device, buildSizesInfoTLAS.buildScratchSize,
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
    memcpy(&vktransformMatrix.matrix, &mesh.transformMatrix.matrix, sizeof(mesh.transformMatrix));

    auto accelerationStructureInstance = vk::AccelerationStructureInstanceKHR{
        .transform=vktransformMatrix,
        .instanceCustomIndex=0,
        .mask=0xFF,
        .instanceShaderBindingTableRecordOffset=0,
        .flags=VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR // ek::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable
    };

    accelerationStructureInstance.accelerationStructureReference=device.getAccelerationStructureAddressKHR({
        .accelerationStructure=bottomAccelerationStructure.accelerationStructure}, dynamicDispatchLoader);
    
    topAccelerationStructure.instancesBuffer = createBuffer(physicalDevice, device, sizeof(vk::AccelerationStructureInstanceKHR),
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
        .primitiveCount=2,
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
}

vk::ImageView createImageView (vk::Device& device, const vk::Image& image, const vk::Format& format)
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

VulkanImage createImage(vk::Device& device, vk::PhysicalDevice& physicalDevice, const vk::Format& format,
        const vk::Flags<vk::ImageUsageFlagBits>& usageFlagBits, const uint32_t width, const uint32_t height)
{
    vk::ImageCreateInfo imageCreateInfo = {
        .imageType=vk::ImageType::e2D,
        .format=format,
        .extent={.width=width, .height=height, .depth=1},
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
        .memoryTypeIndex=findMemoryTypeIndex(physicalDevice, memoryRequirements.memoryTypeBits,
                                            vk::MemoryPropertyFlagBits::eDeviceLocal)
    };

    vk::DeviceMemory memory = device.allocateMemory(allocateInfo);
    device.bindImageMemory(image, memory, 0);
    return VulkanImage{
        .image = image,
        .memory=memory,
        .imageView=createImageView(device, image, format)
    };
}

void createDescriptor(vk::PhysicalDevice& physicalDevice, vk::Device& device, uint32_t width, uint32_t height, VulkanImage& renderTargetImage, 
                        VulkanAccelerationStructure& topAccelerationStructure, vk::DescriptorSet& rtDescriptorSet,
                        vk::DescriptorSetLayout& rtDescriptorSetLayout, VulkanBuffer& uniformBuffer)
{
    struct UniformData
    {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
    };
    UniformData uniformData{};
    uniformData.projInverse = glm::inverse(glm::perspective(glm::radians(60.0f), (float)width/(float)height, 0.1f, 1000.0f));
    uniformData.viewInverse = glm::inverse(glm::lookAt(glm::vec3(0.0, 0.0, -2.5), glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0)));
    const vk::DeviceSize uniformBufferSize = sizeof(uniformData);

    uniformBuffer=createBuffer(physicalDevice, device, uniformBufferSize, vk::BufferUsageFlagBits::eUniformBuffer, vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | vk::MemoryPropertyFlagBits::eDeviceLocal, &uniformData);

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
}

vk::ShaderModule createShaderModuleFromPreCompiledSPIRV(vk::Device& device, const std::string& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (file.fail()) LOG("Failed miss shader");
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint32_t> rayMissShaderSource(fileSize / sizeof(uint32_t));

    file.read(reinterpret_cast<char *>(rayMissShaderSource.data()),
                    fileSize);

    file.close();

    vk::ShaderModuleCreateInfo createInfo{.codeSize=(uint32_t)rayMissShaderSource.size() * sizeof(uint32_t), .pCode=rayMissShaderSource.data()};
    return device.createShaderModule(createInfo);
}

void createShaderBindingTable(vk::Device& device, vk::PhysicalDevice& physicalDevice, vk::Pipeline& rtPipeline, vk::DispatchLoaderDynamic& dynamicDispatchLoader, 
                            vk::StridedDeviceAddressRegionKHR& sbtRayGenAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtMissAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtHitAddressRegion)
{
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = getRayTracingProperties(physicalDevice);
        uint32_t baseAlignment = rayTracingProperties.shaderGroupBaseAlignment;
        uint32_t handleSize = rayTracingProperties.shaderGroupHandleSize;

        const uint32_t shaderGroupCount = 3;
        vk::DeviceSize sbtBufferSize = baseAlignment * shaderGroupCount;

        VulkanBuffer shaderBindingTableBuffer = createBuffer(physicalDevice, device, sbtBufferSize,
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
}

void prepareCommandBuffers(std::vector<vk::CommandBuffer>& commandBuffers, VulkanImage& renderTargetImage, uint32_t queueId, vk::Pipeline& rtPipeline,
                            vk::DescriptorSet& rtDescriptorSet, vk::PipelineLayout& rtPipelineLayout,
                            vk::StridedDeviceAddressRegionKHR& sbtRayGenAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtMissAddressRegion,
                            vk::StridedDeviceAddressRegionKHR& sbtHitAddressRegion,
                            const uint32_t width, const uint32_t height, vk::DispatchLoaderDynamic& dynamicDispatchLoader, std::vector<vk::Image>& swapChainImages)
{
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
        
        commandBuffers[nn].traceRaysKHR(sbtRayGenAddressRegion, sbtMissAddressRegion, sbtHitAddressRegion, {},
            width, height, 1, dynamicDispatchLoader);

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
            .extent={.width=width, .height=height, .depth=1}
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
}

void updateUniformBuffer(vk::Device& device, VulkanBuffer& uniformBuffer, UniformData& uniformData)
{
    void* data = device.mapMemory(uniformBuffer.memory, 0, sizeof(uniformBuffer));
    memcpy(data, &uniformData, sizeof(uniformData));
    device.unmapMemory(uniformBuffer.memory);
};


void destroyBuffer(vk::Device& device, const VulkanBuffer& buffer)
{
    device.destroyBuffer(buffer.buffer);
    device.freeMemory(buffer.memory);
};

void destroyAccelerationStructure (vk::Device& device, const VulkanAccelerationStructure& accelerationStructure, vk::DispatchLoaderDynamic& dynamicDispatchLoader)
{
    device.destroyAccelerationStructureKHR(accelerationStructure.accelerationStructure, nullptr, dynamicDispatchLoader);
    destroyBuffer(device, accelerationStructure.structureBuffer);
    destroyBuffer(device, accelerationStructure.scratchBuffer);
    destroyBuffer(device, accelerationStructure.instancesBuffer);
};

void destroyImage(vk::Device& device, const VulkanImage& image)
{
    device.destroyImageView(image.imageView);
    device.destroyImage(image.image);
    device.freeMemory(image.memory);
};
#pragma endregion

#pragma endregion //vulkan raytrace stuff

#pragma region Mesh Implementation
void Mesh::Prepare(vk::PhysicalDevice& physicalDevice, vk::Device& device)
{
    const vk::BufferUsageFlags usageFlags = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR |
        vk::BufferUsageFlagBits::eShaderDeviceAddress;

    const vk::MemoryPropertyFlags memoryFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent | 
        vk::MemoryPropertyFlagBits::eDeviceLocal;

    vertexBuffer = createBuffer(physicalDevice, device, vertices.size() * sizeof(Vertex), usageFlags, memoryFlags, vertices.data());
    indexBuffer = createBuffer(physicalDevice, device, indices.size() * sizeof(uint32_t), usageFlags, memoryFlags, indices.data());
    transformBuffer = createBuffer(physicalDevice, device, sizeof(VkTransformMatrixKHR), usageFlags, memoryFlags, &transformMatrix);

    vertexBufferDeviceAddress = vk::DeviceOrHostAddressConstKHR {.deviceAddress=vertexBuffer.address};
    indexBufferDeviceAddress = vk::DeviceOrHostAddressConstKHR {.deviceAddress=indexBuffer.address};
    transformBufferDeviceAddress = vk::DeviceOrHostAddressConstKHR {.deviceAddress=transformBuffer.address};

    geometryBLAS = vk::AccelerationStructureGeometryKHR{
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
}
#pragma endregion

#pragma region Shader Implementation
Shader::Shader(const std::string& _p, Shader::ShaderType _st, vk::Device& _d, uint32_t shaderIndex)
{
    path = _p;
    device = std::make_shared<vk::Device>(_d);
    shaderType = _st;

    shaderModule = createShaderModuleFromPreCompiledSPIRV(_d,_p);

    vk::ShaderStageFlagBits stageFlags;

    switch(shaderType)
    {
        case Shader::ShaderType::RayGen:
            stageFlags = vk::ShaderStageFlagBits::eRaygenKHR;
            shaderGroupCreateInfo = {.type=vk::RayTracingShaderGroupTypeKHR::eGeneral,                  .generalShader=shaderIndex,
                                    .closestHitShader=VK_SHADER_UNUSED_KHR, .anyHitShader=VK_SHADER_UNUSED_KHR, .intersectionShader=VK_SHADER_UNUSED_KHR};
        break;
        case Shader::ShaderType::ClosestHit:
            stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR;
            shaderGroupCreateInfo = {.type=vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup,      .generalShader=VK_SHADER_UNUSED_KHR,
                                    .closestHitShader=shaderIndex, .anyHitShader=VK_SHADER_UNUSED_KHR, .intersectionShader=VK_SHADER_UNUSED_KHR};
        break;
        case Shader::ShaderType::Miss:
            stageFlags = vk::ShaderStageFlagBits::eMissKHR;
            shaderGroupCreateInfo = {.type=vk::RayTracingShaderGroupTypeKHR::eGeneral,                  .generalShader=shaderIndex,
                                    .closestHitShader=VK_SHADER_UNUSED_KHR, .anyHitShader=VK_SHADER_UNUSED_KHR, .intersectionShader=VK_SHADER_UNUSED_KHR};
        break;
        default:
            std::cerr << "Unknown shader type" << std::endl;
            throw std::runtime_error("Unknown Shader Type");
    }

    shaderStageCreateInfo =  {.stage=stageFlags, .module=shaderModule, .pName="main"};

    std::cout << "Loaded shader: " << path << std::endl;
}

Shader::Shader(const Shader& other)
{
    path = other.path;
    device = other.device;
    shaderType = other.shaderType;

    shaderModule = other.shaderModule;

    shaderGroupCreateInfo = other.shaderGroupCreateInfo;
    shaderStageCreateInfo =  other.shaderStageCreateInfo;
}

void Shader::UnloadShaderModule()
{
    device->destroyShaderModule(shaderModule);
    LOG("Unloaded shader module " + path);
}
#pragma endregion

#pragma region RaytracePipeline
RayTracingPipeline::RayTracingPipeline(vk::Device& _d)
{
    device = std::make_unique<vk::Device>(_d);
    shaders = {};
}

void RayTracingPipeline::CreatePipeline(vk::PhysicalDevice& physicalDevice, vk::DescriptorSetLayout& rtDescriptorSetLayout, vk::DispatchLoaderDynamic& dynamicDispatchLoader, uint32_t maxRayRecursionDepth)
{
    LOG("Creating Ray Tracing Pipeline");

    // prepare stages and groups
    std::vector<vk::PipelineShaderStageCreateInfo> stages = {};
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> groups = {};
    for(int i = 0; i < shaders.size(); i++)
    {
        Shader shader = shaders[i];
        stages.push_back(shaders[i].shaderStageCreateInfo);
        groups.push_back(shaders[i].shaderGroupCreateInfo);
    }
    // prepare rt pipeline layout
    pipelineLayout = device->createPipelineLayout( //vk::PipelineLayout
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
        .layout=pipelineLayout,
        .basePipelineHandle=VK_NULL_HANDLE,
        .basePipelineIndex=0
    };

    pipeline = device->createRayTracingPipelineKHR(nullptr, nullptr, pipelineCreateInfo, nullptr, dynamicDispatchLoader).value;
    // TODO: clean up shader modules after pipeline creation
    for(int i = 0; i < shaders.size(); i++)
    {
        Shader shader = shaders[i];
        shader.UnloadShaderModule();
    }

    LOG("Finished creating Ray Tracing Pipeline");
}

void RayTracingPipeline::AddShader(const std::string& path, Shader::ShaderType shaderType)
{
    shaders.push_back(Shader(path, shaderType, *device, shaders.size()));
    LOG("Adding shader to ray tracing pipeline: " + path);
}

void RayTracingPipeline::DestroyPipeline()
{
    LOG("Destroying Ray Tracing Pipeline");
    device->destroyPipeline(pipeline);
    device->destroyPipelineLayout(pipelineLayout);
    LOG("Finished destroying Ray Tracing Pipeline");
}
#pragma endregion

#pragma region Ray Tracing Acceleration Structure implementation
RayTracingAccelerationStructure::RayTracingAccelerationStructure()
{
}

void RayTracingAccelerationStructure::AddMesh(Mesh& mesh, vk::PhysicalDevice& physicalDevice, vk::Device& device, vk::DispatchLoaderDynamic& dynamicDispatchLoader, vk::CommandPool& commandPool, 
                                            vk::Queue& computePresentQueue)
{
    VulkanAccelerationStructure topAS, botAS;
    BuildRTAS(mesh, physicalDevice, device, dynamicDispatchLoader, commandPool, computePresentQueue, topAS, botAS);
    RTASData data = {.mesh=mesh, .topAccelerationStructure=topAS, .bottomAccelerationStructure=botAS};
    rtasDatas.push_back(data);
}

void RayTracingAccelerationStructure::Clear()
{
    rtasDatas.clear();
}

void RayTracingAccelerationStructure::Destroy(vk::Device& device, vk::DispatchLoaderDynamic& dynamicDispatchLoader)
{
    for(int i =0; i < rtasDatas.size(); i++)
    {
        destroyAccelerationStructure(device, rtasDatas[i].bottomAccelerationStructure, dynamicDispatchLoader);
        destroyAccelerationStructure(device, rtasDatas[i].topAccelerationStructure, dynamicDispatchLoader);
    }
}
#pragma endregion

#endif