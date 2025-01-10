#include "app.hpp"
#include <iostream>

#ifdef APP_HPP

#define LOG(f) std::cout << f << std::endl

App::App()
{
    LOG("App init");
    window = std::make_unique<Window>(Window("App", 500, 500));

    
    vulkanRayTraceStuff = std::make_unique<VulkanRaytraceStuff> ();
    vulkanRayTraceStuff->init("App", window->requiredInstanceExtensions);
}

int App::run()
{
    LOG("App run");
    return 0;
}

App::~App()
{
    LOG("App dispose");
}
#endif
