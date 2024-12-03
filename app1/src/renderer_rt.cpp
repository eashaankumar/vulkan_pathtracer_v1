#include "renderer_rt.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>
#include <string>
// #include <vulkan/vulkan_win32.h>

#ifdef RENDERER_RT
#define VALIDATION_ENABLED
#define PLATFORM_WINDOWS
#define APP_NAME "vulkan raytracing learning"

#if defined(VALIDATION_ENABLED)
#define STRING_RESET "\033[0m"
#define STRING_INFO "\033[37m"
#define STRING_WARNING "\033[33m"
#define STRING_ERROR "\033[36m"

struct Vertex
{
public:
    float x, y, z;
};

Vertex CreateVertex(float _x, float _y, float _z)
{
    Vertex v;
    v.x = _x;
    v.y = _y;
    v.z = _z;
    return v;
}

VkBool32
debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
              VkDebugUtilsMessageTypeFlagsEXT messageTypes,
              const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
              void *pUserData) {

  std::string message = pCallbackData->pMessage;

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    message = STRING_INFO + message + STRING_RESET;
    std::cout << message.c_str() << std::endl;
  }

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    message = STRING_WARNING + message + STRING_RESET;
    std::cerr << message.c_str() << std::endl;
  }

  if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    message = STRING_ERROR + message + STRING_RESET;
    std::cerr << message.c_str() << std::endl;
  }

  return VK_FALSE;
}
#endif

void throwExceptionVulkanAPI(VkResult result, const std::string &functionName) {
  std::string message = "Vulkan API exception: return code " +
                        std::to_string(result) + " (" + functionName + ")";

  std::cerr << message.c_str() << std::endl;

  throw std::runtime_error(message);
}

#define LOG(x) std::cout <<(x) << std::endl

RendererRT::RendererRT()
{
    LOG("Renderer RT");
    #pragma region Validation Setup
    VkResult result;
    VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCreateInfoPtr = NULL;
    #if defined(VALIDATION_ENABLED)
    std::vector<VkValidationFeatureEnableEXT> validationFeatureEnableList = {
        // VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};

    VkDebugUtilsMessageSeverityFlagBitsEXT debugUtilsMessageSeverityFlagBits =
        (VkDebugUtilsMessageSeverityFlagBitsEXT)(
            // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

    VkDebugUtilsMessageTypeFlagBitsEXT debugUtilsMessageTypeFlagBits =
        (VkDebugUtilsMessageTypeFlagBitsEXT)(
            // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

    VkValidationFeaturesEXT validationFeatures = {
        .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
        .pNext = NULL,
        .enabledValidationFeatureCount =
            (uint32_t)validationFeatureEnableList.size(),
        .pEnabledValidationFeatures = validationFeatureEnableList.data(),
        .disabledValidationFeatureCount = 0,
        .pDisabledValidationFeatures = NULL};

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .pNext = &validationFeatures,
        .flags = 0,
        .messageSeverity =
            (VkDebugUtilsMessageSeverityFlagsEXT)debugUtilsMessageSeverityFlagBits,
        .messageType =
            (VkDebugUtilsMessageTypeFlagsEXT)debugUtilsMessageTypeFlagBits,
        .pfnUserCallback = &debugCallback,
        .pUserData = NULL};

    debugUtilsMessengerCreateInfoPtr = &debugUtilsMessengerCreateInfo;
    #endif

    VkApplicationInfo applicationInfo = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = APP_NAME,
    .applicationVersion = VK_MAKE_VERSION(1, 0, 0),
    .pEngineName = "",
    .engineVersion = VK_MAKE_VERSION(1, 0, 0),
    .apiVersion = VK_API_VERSION_1_3};

    std::vector<const char *> instanceLayerList = {};
    std::vector<const char *> instanceExtensionList = {};

    #if defined(VALIDATION_ENABLED)
    instanceLayerList.push_back("VK_LAYER_KHRONOS_validation");
    instanceExtensionList.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    #endif
    #pragma endregion

    #pragma region Window Surface
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    SDL_Window* window = SDL_CreateWindow(APP_NAME, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 640 * 2, 360 * 2, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    uint32_t extensionCount;
    const char** extensionNames = 0;
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, nullptr);
    extensionNames = new const char *[extensionCount];
    SDL_Vulkan_GetInstanceExtensions(window, &extensionCount, extensionNames);
    // append all SDL based extensions to total extension name list
    for(int i = 0; i < extensionCount; i++)
    {
        instanceExtensionList.push_back(extensionNames[i]);
    }
    
    for(int i = 0; i < instanceExtensionList.size(); i++)
    {
        LOG(instanceExtensionList[i]);
    }
    
    VkInstanceCreateInfo instanceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
      .pNext = debugUtilsMessengerCreateInfoPtr,
      .flags = 0,
      .pApplicationInfo = &applicationInfo,
      .enabledLayerCount = (uint32_t)instanceLayerList.size(),
      .ppEnabledLayerNames = instanceLayerList.data(),
      .enabledExtensionCount = (uint32_t)instanceExtensionList.size(),
      .ppEnabledExtensionNames = instanceExtensionList.data(),
    };

    VkInstance instanceHandle = VK_NULL_HANDLE;
    result = vkCreateInstance(&instanceCreateInfo, NULL, &instanceHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateInstance");
    }

    VkSurfaceKHR surfaceHandle;
    SDL_Vulkan_CreateSurface(window, instanceHandle, &surfaceHandle );
    #pragma endregion

    #pragma region Physical Device
    uint32_t physicalDeviceCount = 0;
    result = vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount, NULL);
    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkEnumeratePhysicalDevices");
    }
    std::vector<VkPhysicalDevice> physicalDeviceHandleList(physicalDeviceCount);
    result = vkEnumeratePhysicalDevices(instanceHandle, &physicalDeviceCount, physicalDeviceHandleList.data());
    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkEnumeratePhysicalDevices");
    }

    VkPhysicalDevice activePhysicalDeviceHandle = physicalDeviceHandleList[0];

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(activePhysicalDeviceHandle,&physicalDeviceProperties);
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR
      physicalDeviceRayTracingPipelineProperties = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
          .pNext = NULL};
    VkPhysicalDeviceProperties2 physicalDeviceProperties2 = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &physicalDeviceRayTracingPipelineProperties,
      .properties = physicalDeviceProperties};
    vkGetPhysicalDeviceProperties2(activePhysicalDeviceHandle,
                                 &physicalDeviceProperties2);
    VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
    vkGetPhysicalDeviceMemoryProperties(activePhysicalDeviceHandle,
                                      &physicalDeviceMemoryProperties);
    LOG(physicalDeviceProperties.deviceName);

    #pragma region Physical Device Features
    VkPhysicalDeviceBufferDeviceAddressFeatures
      physicalDeviceBufferDeviceAddressFeatures = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
          .pNext = NULL,
          .bufferDeviceAddress = VK_TRUE,
          .bufferDeviceAddressCaptureReplay = VK_FALSE,
          .bufferDeviceAddressMultiDevice = VK_FALSE};
    VkPhysicalDeviceAccelerationStructureFeaturesKHR
      physicalDeviceAccelerationStructureFeatures = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
          .pNext = &physicalDeviceBufferDeviceAddressFeatures,
          .accelerationStructure = VK_TRUE,
          .accelerationStructureCaptureReplay = VK_FALSE,
          .accelerationStructureIndirectBuild = VK_FALSE,
          .accelerationStructureHostCommands = VK_FALSE,
          .descriptorBindingAccelerationStructureUpdateAfterBind = VK_FALSE};
    VkPhysicalDeviceRayTracingPipelineFeaturesKHR
      physicalDeviceRayTracingPipelineFeatures = {
          .sType =
              VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
          .pNext = &physicalDeviceAccelerationStructureFeatures,
          .rayTracingPipeline = VK_TRUE,
          .rayTracingPipelineShaderGroupHandleCaptureReplay = VK_FALSE,
          .rayTracingPipelineShaderGroupHandleCaptureReplayMixed = VK_FALSE,
          .rayTracingPipelineTraceRaysIndirect = VK_FALSE,
          .rayTraversalPrimitiveCulling = VK_FALSE};
    VkPhysicalDeviceFeatures deviceFeatures = {.geometryShader = VK_TRUE};
    #pragma endregion

    #pragma region Physical Device Submission Queue Families
    uint32_t queueFamilyPropertyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(activePhysicalDeviceHandle,
                                            &queueFamilyPropertyCount, NULL);

    std::vector<VkQueueFamilyProperties> queueFamilyPropertiesList(
        queueFamilyPropertyCount);

    vkGetPhysicalDeviceQueueFamilyProperties(activePhysicalDeviceHandle,
                                            &queueFamilyPropertyCount,
                                            queueFamilyPropertiesList.data());
    uint32_t queueFamilyIndex = -1;
    for (uint32_t x = 0; x < queueFamilyPropertiesList.size(); x++) {
        if (queueFamilyPropertiesList[x].queueFlags & VK_QUEUE_GRAPHICS_BIT) {

        VkBool32 isPresentSupported = false;
        result = vkGetPhysicalDeviceSurfaceSupportKHR(
            activePhysicalDeviceHandle, x, surfaceHandle, &isPresentSupported);

        if (result != VK_SUCCESS) {
            throwExceptionVulkanAPI(result, "vkGetPhysicalDeviceSurfaceSupportKHR");
        }

        if (isPresentSupported) {
            queueFamilyIndex = x;
            break;
        }
        }
    }
    std::vector<float> queuePrioritiesList = {1.0f};
    VkDeviceQueueCreateInfo deviceQueueCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .queueFamilyIndex = queueFamilyIndex,
      .queueCount = 1,
      .pQueuePriorities = queuePrioritiesList.data()};
    #pragma endregion

    #pragma region Logical Device
    std::vector<const char *> deviceExtensionList = {
      "VK_KHR_ray_tracing_pipeline",
      "VK_KHR_acceleration_structure",
      "VK_EXT_descriptor_indexing",
      "VK_KHR_maintenance3",
      "VK_KHR_buffer_device_address",
      "VK_KHR_deferred_host_operations",
      "VK_KHR_swapchain"};
    VkDeviceCreateInfo deviceCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .pNext = &physicalDeviceRayTracingPipelineFeatures,
      .flags = 0,
      .queueCreateInfoCount = 1,
      .pQueueCreateInfos = &deviceQueueCreateInfo,
      .enabledLayerCount = 0,
      .ppEnabledLayerNames = NULL,
      .enabledExtensionCount = (uint32_t)deviceExtensionList.size(),
      .ppEnabledExtensionNames = deviceExtensionList.data(),
      .pEnabledFeatures = &deviceFeatures};

    VkDevice deviceHandle = VK_NULL_HANDLE;
    result = vkCreateDevice(activePhysicalDeviceHandle, &deviceCreateInfo, NULL,
                          &deviceHandle);
    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateDevice");
    }
    #pragma endregion
    
    #pragma region Submission Queue
    VkQueue queueHandle = VK_NULL_HANDLE;
    vkGetDeviceQueue(deviceHandle, queueFamilyIndex, 0, &queueHandle);
    #pragma endregion

    #pragma region Device Pointer Functions
    PFN_vkGetBufferDeviceAddressKHR pvkGetBufferDeviceAddressKHR =
      (PFN_vkGetBufferDeviceAddressKHR)vkGetDeviceProcAddr(
          deviceHandle, "vkGetBufferDeviceAddressKHR");

    PFN_vkCreateRayTracingPipelinesKHR pvkCreateRayTracingPipelinesKHR =
        (PFN_vkCreateRayTracingPipelinesKHR)vkGetDeviceProcAddr(
            deviceHandle, "vkCreateRayTracingPipelinesKHR");

    PFN_vkGetAccelerationStructureBuildSizesKHR
        pvkGetAccelerationStructureBuildSizesKHR =
            (PFN_vkGetAccelerationStructureBuildSizesKHR)vkGetDeviceProcAddr(
                deviceHandle, "vkGetAccelerationStructureBuildSizesKHR");

    PFN_vkCreateAccelerationStructureKHR pvkCreateAccelerationStructureKHR =
        (PFN_vkCreateAccelerationStructureKHR)vkGetDeviceProcAddr(
            deviceHandle, "vkCreateAccelerationStructureKHR");

    PFN_vkDestroyAccelerationStructureKHR pvkDestroyAccelerationStructureKHR =
        (PFN_vkDestroyAccelerationStructureKHR)vkGetDeviceProcAddr(
            deviceHandle, "vkDestroyAccelerationStructureKHR");

    PFN_vkGetAccelerationStructureDeviceAddressKHR
        pvkGetAccelerationStructureDeviceAddressKHR =
            (PFN_vkGetAccelerationStructureDeviceAddressKHR)vkGetDeviceProcAddr(
                deviceHandle, "vkGetAccelerationStructureDeviceAddressKHR");

    PFN_vkCmdBuildAccelerationStructuresKHR pvkCmdBuildAccelerationStructuresKHR =
        (PFN_vkCmdBuildAccelerationStructuresKHR)vkGetDeviceProcAddr(
            deviceHandle, "vkCmdBuildAccelerationStructuresKHR");

    PFN_vkGetRayTracingShaderGroupHandlesKHR
        pvkGetRayTracingShaderGroupHandlesKHR =
            (PFN_vkGetRayTracingShaderGroupHandlesKHR)vkGetDeviceProcAddr(
                deviceHandle, "vkGetRayTracingShaderGroupHandlesKHR");

    PFN_vkCmdTraceRaysKHR pvkCmdTraceRaysKHR =
        (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(deviceHandle,
                                                    "vkCmdTraceRaysKHR");

    VkMemoryAllocateFlagsInfo memoryAllocateFlagsInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
        .pNext = NULL,
        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
        .deviceMask = 0};

    #pragma endregion

    #pragma region Command Pool
    VkCommandPoolCreateInfo commandPoolCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndex};

    VkCommandPool commandPoolHandle = VK_NULL_HANDLE;
    result = vkCreateCommandPool(deviceHandle, &commandPoolCreateInfo, NULL,
                                &commandPoolHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateCommandPool");
    }
    #pragma endregion
    
    #pragma region Command Buffers
    VkCommandBufferAllocateInfo commandBufferAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .pNext = NULL,
      .commandPool = commandPoolHandle,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 16};

    std::vector<VkCommandBuffer> commandBufferHandleList =
        std::vector<VkCommandBuffer>(16, VK_NULL_HANDLE);

    result = vkAllocateCommandBuffers(deviceHandle, &commandBufferAllocateInfo,
                                        commandBufferHandleList.data());

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkAllocateCommandBuffers");
    }
    #pragma endregion
    
    #pragma region Surface Features
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
        activePhysicalDeviceHandle, surfaceHandle, &surfaceCapabilities);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result,
                                "vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
    }

    uint32_t surfaceFormatCount = 0;
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        activePhysicalDeviceHandle, surfaceHandle, &surfaceFormatCount, NULL);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    }

    std::vector<VkSurfaceFormatKHR> surfaceFormatList(surfaceFormatCount);
    result = vkGetPhysicalDeviceSurfaceFormatsKHR(
        activePhysicalDeviceHandle, surfaceHandle, &surfaceFormatCount,
        surfaceFormatList.data());

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkGetPhysicalDeviceSurfaceFormatsKHR");
    }

    uint32_t presentModeCount = 0;
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        activePhysicalDeviceHandle, surfaceHandle, &presentModeCount, NULL);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result,
                                "vkGetPhysicalDeviceSurfacePresentModesKHR");
    }

    std::vector<VkPresentModeKHR> presentModeList(presentModeCount);
    result = vkGetPhysicalDeviceSurfacePresentModesKHR(
        activePhysicalDeviceHandle, surfaceHandle, &presentModeCount,
        presentModeList.data());

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result,
                                "vkGetPhysicalDeviceSurfacePresentModesKHR");
    }
    #pragma endregion

    #pragma region Swapchain
    VkSwapchainCreateInfoKHR swapchainCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .surface = surfaceHandle,
      .minImageCount = surfaceCapabilities.minImageCount + 1,
      .imageFormat = surfaceFormatList[0].format,
      .imageColorSpace = surfaceFormatList[0].colorSpace,
      .imageExtent = surfaceCapabilities.currentExtent,
      .imageArrayLayers = 1,
      .imageUsage = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex,
      .preTransform = surfaceCapabilities.currentTransform,
      .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
      .presentMode = presentModeList[0],
      .clipped = VK_TRUE,
      .oldSwapchain = VK_NULL_HANDLE};

    VkSwapchainKHR swapchainHandle = VK_NULL_HANDLE;
    result = vkCreateSwapchainKHR(deviceHandle, &swapchainCreateInfo, NULL,
                                    &swapchainHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateSwapchainKHR");
    }
    #pragma endregion

    #pragma region Swapchain Images
    uint32_t swapchainImageCount = 0;
    result = vkGetSwapchainImagesKHR(deviceHandle, swapchainHandle,
                                    &swapchainImageCount, NULL);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkGetSwapchainImagesKHR");
    }

    std::vector<VkImage> swapchainImageHandleList(swapchainImageCount);
    result = vkGetSwapchainImagesKHR(deviceHandle, swapchainHandle,
                                    &swapchainImageCount,
                                    swapchainImageHandleList.data());

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkGetSwapchainImagesKHR");
    }

    std::vector<VkImageView> swapchainImageViewHandleList(swapchainImageCount,
                                                            VK_NULL_HANDLE);

    for (uint32_t x = 0; x < swapchainImageCount; x++) {
        VkImageViewCreateInfo imageViewCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .pNext = NULL,
            .flags = 0,
            .image = swapchainImageHandleList[x],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format = surfaceFormatList[0].format,
            .components = {VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY,
                        VK_COMPONENT_SWIZZLE_IDENTITY},
            .subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}};

        result = vkCreateImageView(deviceHandle, &imageViewCreateInfo, NULL,
                                &swapchainImageViewHandleList[x]);

        if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateImageView");
        }
    }
    #pragma endregion

    #pragma region Descriptor Pool Set
    std::vector<VkDescriptorPoolSize> descriptorPoolSizeList = {
      {.type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
       .descriptorCount = 1},
      {.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = 1},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, .descriptorCount = 4},
      {.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, .descriptorCount = 1}};

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .pNext = NULL,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = 2,
        .poolSizeCount = (uint32_t)descriptorPoolSizeList.size(),
        .pPoolSizes = descriptorPoolSizeList.data()};

    VkDescriptorPool descriptorPoolHandle = VK_NULL_HANDLE;
    result = vkCreateDescriptorPool(deviceHandle, &descriptorPoolCreateInfo, NULL,
                                    &descriptorPoolHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateDescriptorPool");
    }
    #pragma endregion

    #pragma region Descriptor Pool Set Layout
    std::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindingList = {
      {.binding = 0,
       .descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR,
       .descriptorCount = 1,
       .stageFlags =
           VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
       .pImmutableSamplers = NULL},
      {.binding = 1,
       .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
       .descriptorCount = 1,
       .stageFlags =
           VK_SHADER_STAGE_RAYGEN_BIT_KHR | VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
       .pImmutableSamplers = NULL},
      {.binding = 2,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
       .pImmutableSamplers = NULL},
      {.binding = 3,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
       .pImmutableSamplers = NULL},
      {.binding = 4,
       .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
       .descriptorCount = 1,
       .stageFlags = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
       .pImmutableSamplers = NULL}};

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = (uint32_t)descriptorSetLayoutBindingList.size(),
        .pBindings = descriptorSetLayoutBindingList.data()};

    VkDescriptorSetLayout descriptorSetLayoutHandle = VK_NULL_HANDLE;
    result =
        vkCreateDescriptorSetLayout(deviceHandle, &descriptorSetLayoutCreateInfo,
                                    NULL, &descriptorSetLayoutHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateDescriptorSetLayout");
    }
    #pragma endregion

    #pragma region Material Descriptor Set Layout
    std::vector<VkDescriptorSetLayoutBinding>
      materialDescriptorSetLayoutBindingList = {
          {.binding = 0,
           .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
           .descriptorCount = 1,
           .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
           .pImmutableSamplers = NULL},
          {.binding = 1,
           .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
           .descriptorCount = 1,
           .stageFlags = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
           .pImmutableSamplers = NULL}};

    VkDescriptorSetLayoutCreateInfo materialDescriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .bindingCount = (uint32_t)materialDescriptorSetLayoutBindingList.size(),
        .pBindings = materialDescriptorSetLayoutBindingList.data()};

    VkDescriptorSetLayout materialDescriptorSetLayoutHandle = VK_NULL_HANDLE;
    result = vkCreateDescriptorSetLayout(
        deviceHandle, &materialDescriptorSetLayoutCreateInfo, NULL,
        &materialDescriptorSetLayoutHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateDescriptorSetLayout");
    }

    #pragma endregion

    #pragma region Allocator Descriptor Set
    std::vector<VkDescriptorSetLayout> descriptorSetLayoutHandleList = {
      descriptorSetLayoutHandle, materialDescriptorSetLayoutHandle};

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = NULL,
        .descriptorPool = descriptorPoolHandle,
        .descriptorSetCount = (uint32_t)descriptorSetLayoutHandleList.size(),
        .pSetLayouts = descriptorSetLayoutHandleList.data()};

    std::vector<VkDescriptorSet> descriptorSetHandleList =
        std::vector<VkDescriptorSet>(2, VK_NULL_HANDLE);

    result = vkAllocateDescriptorSets(deviceHandle, &descriptorSetAllocateInfo,
                                        descriptorSetHandleList.data());

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkAllocateDescriptorSets");
    }
    #pragma endregion

    #pragma region Pipeline Layout
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .setLayoutCount = (uint32_t)descriptorSetLayoutHandleList.size(),
      .pSetLayouts = descriptorSetLayoutHandleList.data(),
      .pushConstantRangeCount = 0,
      .pPushConstantRanges = NULL};

    VkPipelineLayout pipelineLayoutHandle = VK_NULL_HANDLE;
    result = vkCreatePipelineLayout(deviceHandle, &pipelineLayoutCreateInfo, NULL,
                                    &pipelineLayoutHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreatePipelineLayout");
    }

    #pragma endregion
    
    #pragma region Ray Closest Hit Shader Module

    std::ifstream rayClosestHitFile("compiled_shaders/shader.rchit.spv",
                                  std::ios::binary | std::ios::ate);
    if (rayClosestHitFile.fail()) LOG("FAILED TO OPEN FILE");

    std::streamsize rayClosestHitFileSize = rayClosestHitFile.tellg();
    rayClosestHitFile.seekg(0, std::ios::beg);
    std::vector<uint32_t> rayClosestHitShaderSource(rayClosestHitFileSize /
                                                    sizeof(uint32_t));
    rayClosestHitFile.read(
        reinterpret_cast<char *>(rayClosestHitShaderSource.data()),
        rayClosestHitFileSize);
    rayClosestHitFile.close();

    VkShaderModuleCreateInfo rayClosestHitShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = (uint32_t)rayClosestHitShaderSource.size() * sizeof(uint32_t),
        .pCode = rayClosestHitShaderSource.data()};

    VkShaderModule rayClosestHitShaderModuleHandle = VK_NULL_HANDLE;
    result =
        vkCreateShaderModule(deviceHandle, &rayClosestHitShaderModuleCreateInfo,
                            NULL, &rayClosestHitShaderModuleHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateShaderModule");
    }

    #pragma endregion

    #pragma region Ray Generator Shader Module
    std::ifstream rayGenerateFile("compiled_shaders/shader.rgen.spv",
                                std::ios::binary | std::ios::ate);
    if (rayGenerateFile.fail()) LOG("FAILED TO OPEN shader gen");
    std::streamsize rayGenerateFileSize = rayGenerateFile.tellg();
    rayGenerateFile.seekg(0, std::ios::beg);
    std::vector<uint32_t> rayGenerateShaderSource(rayGenerateFileSize /
                                                    sizeof(uint32_t));

    rayGenerateFile.read(reinterpret_cast<char *>(rayGenerateShaderSource.data()),
                        rayGenerateFileSize);

    rayGenerateFile.close();

    VkShaderModuleCreateInfo rayGenerateShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = (uint32_t)rayGenerateShaderSource.size() * sizeof(uint32_t),
        .pCode = rayGenerateShaderSource.data()};

    VkShaderModule rayGenerateShaderModuleHandle = VK_NULL_HANDLE;
    result =
        vkCreateShaderModule(deviceHandle, &rayGenerateShaderModuleCreateInfo,
                            NULL, &rayGenerateShaderModuleHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateShaderModule");
    }

    #pragma endregion

    #pragma region Shader Miss Module
    std::ifstream rayMissFile("compiled_shaders/shader.rmiss.spv",
                            std::ios::binary | std::ios::ate);
    if (rayMissFile.fail()) LOG("Failed miss shader");
    std::streamsize rayMissFileSize = rayMissFile.tellg();
    rayMissFile.seekg(0, std::ios::beg);
    std::vector<uint32_t> rayMissShaderSource(rayMissFileSize / sizeof(uint32_t));

    rayMissFile.read(reinterpret_cast<char *>(rayMissShaderSource.data()),
                    rayMissFileSize);

    rayMissFile.close();

    VkShaderModuleCreateInfo rayMissShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = (uint32_t)rayMissShaderSource.size() * sizeof(uint32_t),
        .pCode = rayMissShaderSource.data()};

    VkShaderModule rayMissShaderModuleHandle = VK_NULL_HANDLE;
    result = vkCreateShaderModule(deviceHandle, &rayMissShaderModuleCreateInfo,
                                    NULL, &rayMissShaderModuleHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateShaderModule");
    }

    #pragma endregion

    #pragma region Shader Miss Shadow Module
    std::ifstream rayMissShadowFile("compiled_shaders/shader_shadow.rmiss.spv",
                                  std::ios::binary | std::ios::ate);
    if (rayMissShadowFile.fail()) LOG("Failed shadow shader");
    std::streamsize rayMissShadowFileSize = rayMissShadowFile.tellg();
    rayMissShadowFile.seekg(0, std::ios::beg);
    std::vector<uint32_t> rayMissShadowShaderSource(rayMissShadowFileSize /
                                                    sizeof(uint32_t));

    rayMissShadowFile.read(
        reinterpret_cast<char *>(rayMissShadowShaderSource.data()),
        rayMissShadowFileSize);

    rayMissShadowFile.close();

    VkShaderModuleCreateInfo rayMissShadowShaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = NULL,
        .flags = 0,
        .codeSize = (uint32_t)rayMissShadowShaderSource.size() * sizeof(uint32_t),
        .pCode = rayMissShadowShaderSource.data()};

    VkShaderModule rayMissShadowShaderModuleHandle = VK_NULL_HANDLE;
    result =
        vkCreateShaderModule(deviceHandle, &rayMissShadowShaderModuleCreateInfo,
                            NULL, &rayMissShadowShaderModuleHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateShaderModule");
    }

    #pragma endregion

    #pragma region Raytracing Pipeline
    std::vector<VkPipelineShaderStageCreateInfo>
      pipelineShaderStageCreateInfoList = {
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
           .module = rayClosestHitShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_RAYGEN_BIT_KHR,
           .module = rayGenerateShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
           .module = rayMissShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL},
          {.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
           .pNext = NULL,
           .flags = 0,
           .stage = VK_SHADER_STAGE_MISS_BIT_KHR,
           .module = rayMissShadowShaderModuleHandle,
           .pName = "main",
           .pSpecializationInfo = NULL}};

    std::vector<VkRayTracingShaderGroupCreateInfoKHR>
      rayTracingShaderGroupCreateInfoList = {
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR,
           .generalShader = VK_SHADER_UNUSED_KHR,
           .closestHitShader = 0,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL},
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
           .generalShader = 1,
           .closestHitShader = VK_SHADER_UNUSED_KHR,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL},
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
           .generalShader = 2,
           .closestHitShader = VK_SHADER_UNUSED_KHR,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL},
          {.sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR,
           .pNext = NULL,
           .type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR,
           .generalShader = 3,
           .closestHitShader = VK_SHADER_UNUSED_KHR,
           .anyHitShader = VK_SHADER_UNUSED_KHR,
           .intersectionShader = VK_SHADER_UNUSED_KHR,
           .pShaderGroupCaptureReplayHandle = NULL}};
    
    VkRayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
      .pNext = NULL,
      .flags = 0,
      .stageCount = 4,
      .pStages = pipelineShaderStageCreateInfoList.data(),
      .groupCount = 4,
      .pGroups = rayTracingShaderGroupCreateInfoList.data(),
      .maxPipelineRayRecursionDepth = 1,
      .pLibraryInfo = NULL,
      .pLibraryInterface = NULL,
      .pDynamicState = NULL,
      .layout = pipelineLayoutHandle,
      .basePipelineHandle = VK_NULL_HANDLE,
      .basePipelineIndex = 0};

    VkPipeline rayTracingPipelineHandle = VK_NULL_HANDLE;
    result = pvkCreateRayTracingPipelinesKHR(
        deviceHandle, VK_NULL_HANDLE, VK_NULL_HANDLE, 1,
        &rayTracingPipelineCreateInfo, NULL, &rayTracingPipelineHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateRayTracingPipelinesKHR");
    }
    #pragma endregion

    #pragma region Vertices
    std::vector<uint32_t> indexList;
    std::vector<Vertex> vertices;

    vertices.push_back(CreateVertex(-1, -1, 0));
    vertices.push_back(CreateVertex(1, -1, 0));
    vertices.push_back(CreateVertex(1, 1, 0));
    vertices.push_back(CreateVertex(-1, 1, 0));

    indexList.push_back(0);
    indexList.push_back(1);
    indexList.push_back(2);

    indexList.push_back(2);
    indexList.push_back(3);
    indexList.push_back(0);

    #pragma endregion

    #pragma region Vertex Buffer
    VkBufferCreateInfo vertexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(float) * vertices.size() * 3,
      .usage =
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex};

    VkBuffer vertexBufferHandle = VK_NULL_HANDLE;
    result = vkCreateBuffer(deviceHandle, &vertexBufferCreateInfo, NULL,
                          &vertexBufferHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateBuffer");
    }

    VkMemoryRequirements vertexMemoryRequirements;
    vkGetBufferMemoryRequirements(deviceHandle, vertexBufferHandle,
                                &vertexMemoryRequirements);

    uint32_t vertexMemoryTypeIndex = -1;
    for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount; x++) {
    if ((vertexMemoryRequirements.memoryTypeBits & (1 << x)) &&
        (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
         VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

            vertexMemoryTypeIndex = x;
            break;
        }
    }

    VkMemoryAllocateInfo vertexMemoryAllocateInfo = {
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .pNext = &memoryAllocateFlagsInfo,
      .allocationSize = vertexMemoryRequirements.size,
      .memoryTypeIndex = vertexMemoryTypeIndex};

    VkDeviceMemory vertexDeviceMemoryHandle = VK_NULL_HANDLE;
    result = vkAllocateMemory(deviceHandle, &vertexMemoryAllocateInfo, NULL,
                            &vertexDeviceMemoryHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkAllocateMemory");
    }

    result = vkBindBufferMemory(deviceHandle, vertexBufferHandle,
                                vertexDeviceMemoryHandle, 0);
    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkBindBufferMemory");
    }

    void *hostVertexMemoryBuffer;
    result = vkMapMemory(deviceHandle, vertexDeviceMemoryHandle, 0,
                        sizeof(float) * vertices.size() * 3, 0,
                        &hostVertexMemoryBuffer);

    memcpy(hostVertexMemoryBuffer, vertices.data(),
            sizeof(float) * vertices.size() * 3);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkMapMemory");
    }

    vkUnmapMemory(deviceHandle, vertexDeviceMemoryHandle);

    VkBufferDeviceAddressInfo vertexBufferDeviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = vertexBufferHandle};

    VkDeviceAddress vertexBufferDeviceAddress = pvkGetBufferDeviceAddressKHR(
        deviceHandle, &vertexBufferDeviceAddressInfo);

    #pragma endregion

    #pragma region Index Buffer
    VkBufferCreateInfo indexBufferCreateInfo = {
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .pNext = NULL,
      .flags = 0,
      .size = sizeof(uint32_t) * indexList.size(),
      .usage =
          VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
          VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR |
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
      .queueFamilyIndexCount = 1,
      .pQueueFamilyIndices = &queueFamilyIndex};

    VkBuffer indexBufferHandle = VK_NULL_HANDLE;
    result = vkCreateBuffer(deviceHandle, &indexBufferCreateInfo, NULL,
                            &indexBufferHandle);

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkCreateBuffer");
    }

    VkMemoryRequirements indexMemoryRequirements;
    vkGetBufferMemoryRequirements(deviceHandle, indexBufferHandle,
                                    &indexMemoryRequirements);

    uint32_t indexMemoryTypeIndex = -1;
    for (uint32_t x = 0; x < physicalDeviceMemoryProperties.memoryTypeCount;
        x++) {
        if ((indexMemoryRequirements.memoryTypeBits & (1 << x)) &&
            (physicalDeviceMemoryProperties.memoryTypes[x].propertyFlags &
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) ==
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {

        indexMemoryTypeIndex = x;
        break;
        }
    }

    VkMemoryAllocateInfo indexMemoryAllocateInfo = {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &memoryAllocateFlagsInfo,
        .allocationSize = indexMemoryRequirements.size,
        .memoryTypeIndex = indexMemoryTypeIndex};

    VkDeviceMemory indexDeviceMemoryHandle = VK_NULL_HANDLE;
    result = vkAllocateMemory(deviceHandle, &indexMemoryAllocateInfo, NULL,
                                &indexDeviceMemoryHandle);
    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkAllocateMemory");
    }

    result = vkBindBufferMemory(deviceHandle, indexBufferHandle,
                                indexDeviceMemoryHandle, 0);
    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkBindBufferMemory");
    }

    void *hostIndexMemoryBuffer;
    result = vkMapMemory(deviceHandle, indexDeviceMemoryHandle, 0,
                        sizeof(uint32_t) * indexList.size(), 0,
                        &hostIndexMemoryBuffer);

    memcpy(hostIndexMemoryBuffer, indexList.data(),
            sizeof(uint32_t) * indexList.size());

    if (result != VK_SUCCESS) {
        throwExceptionVulkanAPI(result, "vkMapMemory");
    }

    vkUnmapMemory(deviceHandle, indexDeviceMemoryHandle);

    VkBufferDeviceAddressInfo indexBufferDeviceAddressInfo = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = NULL,
        .buffer = indexBufferHandle};

    VkDeviceAddress indexBufferDeviceAddress =
        pvkGetBufferDeviceAddressKHR(deviceHandle, &indexBufferDeviceAddressInfo);
    #pragma endregion

    #pragma endregion
}
#endif