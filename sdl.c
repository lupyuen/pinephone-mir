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
Starting program: /usr/share/click/preinstalled/com.ubuntu.filemanager/0.7.5/sdl 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
Init SDL...
[New Thread 0xfffff7b97200 (LWP 11002)]
[3530130.318]  -> wl_display@1.get_registry(new id wl_registry@2)
[3530130.502]  -> wl_display@1.sync(new id wl_callback@3)
[3530183.102] wl_display@1.delete_id(3)
[3530183.260] wl_registry@2.global(1, "wl_drm", 2)
WAYLAND INTERFACE: wl_drm
[3530183.395] wl_registry@2.global(2, "qt_windowmanager", 1)
WAYLAND INTERFACE: qt_windowmanager
[3530183.519]  -> wl_registry@2.bind(2, "qt_windowmanager", 1, new id [unknown]@4)
[3530183.643] wl_registry@2.global(3, "wl_compositor", 4)
WAYLAND INTERFACE: wl_compositor
[3530183.746]  -> wl_registry@2.bind(3, "wl_compositor", 3, new id [unknown]@5)
[3530183.859] wl_registry@2.global(4, "wl_subcompositor", 1)
WAYLAND INTERFACE: wl_subcompositor
[3530183.950] wl_registry@2.global(5, "wl_seat", 6)
WAYLAND INTERFACE: wl_seat
[3530184.066]  -> wl_registry@2.bind(5, "wl_seat", 5, new id [unknown]@6)
[3530184.231] wl_registry@2.global(6, "wl_output", 3)
WAYLAND INTERFACE: wl_output
[3530184.326]  -> wl_registry@2.bind(6, "wl_output", 2, new id [unknown]@7)
[3530184.454] wl_registry@2.global(7, "wl_data_device_manager", 3)
WAYLAND INTERFACE: wl_data_device_manager
[3530184.553]  -> wl_registry@2.bind(7, "wl_data_device_manager", 3, new id [unknown]@8)
[3530184.662] wl_registry@2.global(8, "wl_shell", 1)
WAYLAND INTERFACE: wl_shell
[3530184.756]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@9)
[3530184.831] wl_registry@2.global(9, "zxdg_shell_v6", 1)
WAYLAND INTERFACE: zxdg_shell_v6
[3530184.953]  -> wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@10)
[3530185.122] wl_registry@2.global(10, "xdg_wm_base", 1)
WAYLAND INTERFACE: xdg_wm_base
[3530185.220]  -> wl_registry@2.bind(10, "xdg_wm_base", 1, new id [unknown]@11)
[3530185.331] wl_registry@2.global(11, "wl_shm", 1)
WAYLAND INTERFACE: wl_shm
[3530185.411]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@12)
[3530185.909]  -> wl_shm@12.create_pool(new id wl_shm_pool@13, fd 5, 4096)
[3530187.941]  -> wl_shm_pool@13.resize(12288)
[3530199.357]  -> wl_shm_pool@13.resize(28672)
[3530200.630]  -> wl_shm_pool@13.resize(61440)
[3530206.888]  -> wl_shm_pool@13.resize(126976)
[3530207.398]  -> wl_shm_pool@13.resize(258048)
[3530214.241]  -> wl_shm_pool@13.resize(520192)
[3530230.581]  -> wl_shm_pool@13.resize(1044480)
[3530278.373] wl_callback@3.done(117)
[3530278.507]  -> wl_display@1.sync(new id wl_callback@3)
[3530278.680] wl_seat@6.capabilities(7)
[3530278.731]  -> wl_seat@6.get_pointer(new id wl_pointer@14)
[3530278.826]  -> wl_seat@6.get_touch(new id wl_touch@15)
[3530278.875]  -> wl_seat@6.get_keyboard(new id wl_keyboard@16)
[3530278.920] wl_seat@6.name("seat0")
[3530279.227] wl_display@1.delete_id(3)
[3530279.420] wl_output@7.geometry(0, 0, 68, 136, 0, "Fake manufacturer", "Fake model", 0)
[3530279.704] wl_output@7.mode(3, 720, 1440, 553)
[3530279.860] wl_output@7.scale(1)
[3530279.894] wl_output@7.done()
[3530279.917] wl_callback@3.done(117)
[3530279.984]  -> wl_shm_pool@13.create_buffer(new id wl_buffer@3, 770048, 32, 32, 128, 0)
[3530280.078]  -> wl_compositor@5.create_surface(new id wl_surface@17)
[3530280.151]  -> wl_pointer@14.set_cursor(0, wl_surface@17, 10, 5)
[3530280.199]  -> wl_surface@17.attach(wl_buffer@3, 0, 0)
[3530280.269]  -> wl_surface@17.damage(0, 0, 32, 32)
[3530280.341]  -> wl_surface@17.commit()
Creating window...
[3530285.815]  -> wl_compositor@5.create_surface(new id wl_surface@18)
[3530285.908]  -> xdg_wm_base@11.get_xdg_surface(new id xdg_surface@19, wl_surface@18)
[3530285.944]  -> xdg_surface@19.get_toplevel(new id xdg_toplevel@20)
[3530285.971]  -> xdg_toplevel@20.set_app_id("sdl")
[3530286.014]  -> xdg_toplevel@20.destroy()
[3530286.050]  -> xdg_surface@19.destroy()
[3530286.070]  -> wl_surface@18.destroy()
sdl: sdl.c:24: main: Assertion `window != NULL' failed.

Thread 1 "sdl" received signal SIGABRT, Aborted.
0x0000fffff7d5e568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
#0  0x0000fffff7d5e568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
#1  0x0000fffff7d5fa20 in abort () from /lib/aarch64-linux-gnu/libc.so.6
#2  0x0000fffff7d57c44 in ?? () from /lib/aarch64-linux-gnu/libc.so.6
#3  0x0000000000400e40 in ?? ()
---Type <return> to continue, or q <return> to quit---Backtrace stopped: previous frame inner to this frame (corrupt stack?)
#0  0x0000fffff7d5e568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
(gdb) quit
*/
