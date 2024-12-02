#include "renderer_rt.hpp"
#include <SDL2/SDL.h>
#include <SDL2/SDL_vulkan.h>
#include <vulkan/vulkan_core.h>
#include <vulkan/vulkan.h>

#include <fstream>
#include <iostream>
#include <vector>

#ifdef RENDERER_RT
#define VALIDATION_ENABLED
RendererRT::RendererRT()
{
    std::cout << "Renderer RT" << std::endl;
    // VkResult result;
    // VkDebugUtilsMessengerCreateInfoEXT *debugUtilsMessengerCreateInfoPtr = NULL;
    #if defined(VALIDATION_ENABLED)
    // std::vector<VkValidationFeatureEnableEXT> validationFeatureEnableList = {
    //     // VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
    //     VK_VALIDATION_FEATURE_ENABLE_DEBUG_PRINTF_EXT,
    //     VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT};

    // VkDebugUtilsMessageSeverityFlagBitsEXT debugUtilsMessageSeverityFlagBits =
    //     (VkDebugUtilsMessageSeverityFlagBitsEXT)(
    //         // VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
    //         VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
    //         VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
    //         VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT);

    // VkDebugUtilsMessageTypeFlagBitsEXT debugUtilsMessageTypeFlagBits =
    //     (VkDebugUtilsMessageTypeFlagBitsEXT)(
    //         // VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
    //         VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
    //         VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT);

    // VkValidationFeaturesEXT validationFeatures = {
    //     .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    //     .pNext = NULL,
    //     .enabledValidationFeatureCount =
    //         (uint32_t)validationFeatureEnableList.size(),
    //     .pEnabledValidationFeatures = validationFeatureEnableList.data(),
    //     .disabledValidationFeatureCount = 0,
    //     .pDisabledValidationFeatures = NULL};

    // VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo = {
    //     .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
    //     .pNext = &validationFeatures,
    //     .flags = 0,
    //     .messageSeverity =
    //         (VkDebugUtilsMessageSeverityFlagsEXT)debugUtilsMessageSeverityFlagBits,
    //     .messageType =
    //         (VkDebugUtilsMessageTypeFlagsEXT)debugUtilsMessageTypeFlagBits,
    //     .pfnUserCallback = &debugCallback,
    //     .pUserData = NULL};

    // debugUtilsMessengerCreateInfoPtr = &debugUtilsMessengerCreateInfo;
    #endif
}
#endif