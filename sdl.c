//  Based on https://www.geeksforgeeks.org/sdl-library-in-c-c-with-examples/
#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include <assert.h>
  
int main() 
{   
    //  Init SDL library
    puts("Init SDL...");
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
        printf("error initializing SDL: %s\n", SDL_GetError()); 
    }

    //  Create window
    puts("Creating window...");
    SDL_Window* window = SDL_CreateWindow("GAME", 
                                       SDL_WINDOWPOS_CENTERED, 
                                       SDL_WINDOWPOS_CENTERED, 
                                       200, 200, 0); 
    assert(window != NULL);

    //  Get window surface
    puts("Getting window surface...");
    SDL_Surface *screenSurface = SDL_GetWindowSurface( window );
    assert(screenSurface != NULL);

    //  Fill the surface cyan
    puts("Filling window surface...");
    SDL_FillRect( screenSurface, NULL, SDL_MapRGB( screenSurface->format, 0x00, 0xFF, 0xFF ) );
    
    //  Update the surface
    puts("Updating window surface...");
    SDL_UpdateWindowSurface( window );

    //  Wait two seconds
    puts("Waiting...");
    SDL_Delay( 2000 );

    return 0; 
}

/* Output:
[840568.338]  -> wl_display@1.get_registry(new id wl_registry@2)
[840568.445]  -> wl_display@1.sync(new id wl_callback@3)
[840571.298]  -> wl_display@1.sync(new id wl_callback@4)

#0  0x0000fffff73ea9cc in wl_cursor_theme_get_cursor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-cursor.so.0
#1  0x0000fffff7f6c9d0 in Wayland_CreateDefaultCursor ()
    at /home/phablet/SDL2-2.0.12/src/video/wayland/SDL_waylandmouse.c:231
#2  Wayland_InitMouse ()
    at /home/phablet/SDL2-2.0.12/src/video/wayland/SDL_waylandmouse.c:386
#3  0x0000fffff7f6d72c in Wayland_VideoInit (_this=<optimized out>)
    at /home/phablet/SDL2-2.0.12/src/video/wayland/SDL_waylandvideo.c:444
#4  0x0000fffff7f5767c in SDL_VideoInit_REAL (
    driver_name=0xfffffffff91f "wayland", driver_name@entry=0x0)
---Type <return> to continue, or q <return> to quit---    at /home/phablet/SDL2-2.0.12/src/video/SDL_video.c:532
#5  0x0000fffff7e9b308 in SDL_InitSubSystem_REAL (flags=62001)
    at /home/phablet/SDL2-2.0.12/src/SDL.c:206
#6  SDL_Init_REAL (flags=<optimized out>)
    at /home/phablet/SDL2-2.0.12/src/SDL.c:291
#7  0x0000000000400ab0 in main () at sdl.c:8
#0  0x0000fffff73ea9cc in wl_cursor_theme_get_cursor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-cursor.so.0
*/