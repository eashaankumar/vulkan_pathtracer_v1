#include <memory>
#include "window.hpp"
#include "vulkan_raytrace_stuff.hpp"

#ifndef APP_HPP
#define APP_HPP
class App{
public:
    App();
    int run();
    ~App();

    std::unique_ptr<Window> window;
    std::unique_ptr<VulkanRaytraceStuff> vulkanRayTraceStuff;
};
#endif