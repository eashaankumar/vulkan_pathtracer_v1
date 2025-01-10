#include "window.hpp"
#include <SDL2/SDL_vulkan.h>
#include <iostream>

#ifdef WINDOW_HPP
void getExtensions(Window* window);

Window::Window(const char* title, int width, int height)
{
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Vulkan_LoadLibrary(nullptr);
    window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    getExtensions(this);
}

void getExtensions(Window* window)
{
    window->requiredInstanceExtensions = {};
    uint32_t extensionCount;
    const char** extensionNames = 0;
    SDL_Vulkan_GetInstanceExtensions(window->window, &extensionCount, nullptr);
    extensionNames = new const char *[extensionCount];
    SDL_Vulkan_GetInstanceExtensions(window->window, &extensionCount, extensionNames);
    // append all SDL based extensions to total extension name list
    for(int i = 0; i < extensionCount; i++)
    {
        window->requiredInstanceExtensions.push_back(extensionNames[i]);
    }
}

Window::~Window()
{
    SDL_DestroyWindow(window);
    SDL_Quit();
}
#endif