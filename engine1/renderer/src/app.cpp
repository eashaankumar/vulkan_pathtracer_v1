#include "app.hpp"
#include <iostream>

#ifdef APP_HPP

#define LOG(f) std::cout << f << std::endl

App::App()
{
    LOG("App init");
    
    // https://aistudio.google.com/prompts/new_chat?gad_source=1&gclid=Cj0KCQiAyoi8BhDvARIsAO_CDsB8cSUUB2p4OtXkKttuaZBj9smU9RrnSYQugPYL6e0YY4FHsGzq6JAaAkklEALw_wcB
    vulkanRayTraceStuff = std::make_unique<VulkanRaytraceStuff> ();
    vulkanRayTraceStuff->init("App", 500, 500);
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
