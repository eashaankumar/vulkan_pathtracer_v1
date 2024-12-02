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
    #pragma endregion
}
#endif