#include <SDL2/SDL.h>
#include <vector>

#ifndef WINDOW_HPP
#define WINDOW_HPP
class Window{
public:
    Window(const char* title, int width, int height);
    ~Window();
    SDL_Window* window;

    std::vector<const char *> requiredInstanceExtensions;
};
#endif
