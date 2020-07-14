//  Based on https://www.geeksforgeeks.org/sdl-library-in-c-c-with-examples/
#include <SDL2/SDL.h> 
#include <SDL2/SDL_timer.h> 
#include <assert.h>
  
int main() 
{   
    //  Set SDL logging
    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
    
    //  Init SDL library
    puts("Init SDL...");
    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) { 
        printf("error initializing SDL: %s\n", SDL_GetError()); 
        exit(1);
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
Init SDL...
[New Thread 0xfffff7c40200 (LWP 29126)]
[3373274.066]  -> wl_display@1.get_registry(new id wl_registry@2)
[3373274.207]  -> wl_display@1.sync(new id wl_callback@3)
[3373275.902]  -> wl_display@1.sync(new id wl_callback@4)

Thread 1 "sdl" received signal SIGSEGV, Segmentation fault.
0x0000fffff73e39cc in wl_cursor_theme_get_cursor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-cursor.so.0
#0  0x0000fffff73e39cc in wl_cursor_theme_get_cursor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-cursor.so.0
#1  0x0000fffff7f67f00 in Wayland_CreateDefaultCursor ()
    at /home/phablet/SDL-ubuntu-touch/src/video/wayland/SDL_waylandmouse.c:233
#2  Wayland_InitMouse ()
    at /home/phablet/SDL-ubuntu-touch/src/video/wayland/SDL_waylandmouse.c:388
#3  0x0000fffff7f68c6c in Wayland_VideoInit (_this=<optimized out>)
    at /home/phablet/SDL-ubuntu-touch/src/video/wayland/SDL_waylandvideo.c:445
#4  0x0000fffff7f53cac in SDL_VideoInit_REAL (
    driver_name=0xfffffffff91f "wayland", driver_name@entry=0x0)
    at /home/phablet/SDL-ubuntu-touch/src/video/SDL_video.c:532
#5  0x0000fffff7e94f48 in SDL_InitSubSystem_REAL (flags=62001)
    at /home/phablet/SDL-ubuntu-touch/src/SDL.c:206
#6  SDL_Init_REAL (flags=<optimized out>)
    at /home/phablet/SDL-ubuntu-touch/src/SDL.c:291
#7  0x0000000000400b5c in main () at sdl.c:10
#0  0x0000fffff73e39cc in wl_cursor_theme_get_cursor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-cursor.so.0
*/
