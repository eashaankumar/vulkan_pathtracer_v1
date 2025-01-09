#include <iostream>
#include "renderer_raster.hpp"
#include "kenwright_minimal_v1.hpp"

// # argv and char** necessary for SDL_main error to go away
int main(int argv, char** args){
    // RendererRT renderer;
    // RendererRaster rendererRaster;
    //Kenwright_1_2 ken12;
    //return ken12.run();

    KenWrightMinimal_V1 kenmin;
    return kenmin.run();
}