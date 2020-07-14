//  Based on https://www.geeksforgeeks.org/sdl-library-in-c-c-with-examples/
#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
  
int main() 
{   
    // retutns zero on success else non-zero 
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
        printf("error initializing SDL: %s\n", SDL_GetError()); 
    } 
    SDL_Window* window = SDL_CreateWindow("GAME", 
                                       SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, 
                                       200, 200, 0); 

    //  Get window surface
    SDL_Surface *screenSurface = SDL_GetWindowSurface( window );

    //  Fill the surface cyan
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x00, 0xFF, 0xFF ) );
    
    //  Update the surface
    SDL_UpdateWindowSurface( window );

    //  Wait two seconds
    SDL_Delay( 2000 );

    return 0; 
}