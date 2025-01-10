#include <iostream>
#include "app.hpp"
#include "kenwright_minimal_v1.hpp"

// # argv and char** necessary for SDL_main error to go away
int main(int argv, char** args){
    App app;
    return app.run();

    // KenWrightMinimal_V1 kne;
    // return kne.run();
}