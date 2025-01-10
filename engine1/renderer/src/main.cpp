#include <iostream>
#include "app.hpp"

// # argv and char** necessary for SDL_main error to go away
int main(int argv, char** args){
    App app;
    return app.run();
}