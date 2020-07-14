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
Reading symbols from ./filemanager.ubuntu.com.filemanager...done.
Starting program: /usr/share/click/preinstalled/com.ubuntu.filemanager/0.7.5/filemanager.ubuntu.com.filemanager 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
Init SDL...
[New Thread 0xfffff7b98200 (LWP 13733)]
[3395216.659]  -> wl_display@1.get_registry(new id wl_registry@2)
[3395216.821]  -> wl_display@1.sync(new id wl_callback@3)
[3395256.434] wl_display@1.delete_id(3)
[3395256.669] wl_registry@2.global(1, "wl_drm", 2)
WAYLAND INTERFACE: wl_drm
[3395256.937] wl_registry@2.global(2, "qt_windowmanager", 1)
WAYLAND INTERFACE: qt_windowmanager
[3395257.252] wl_registry@2.global(3, "wl_compositor", 4)
WAYLAND INTERFACE: wl_compositor
[3395258.252]  -> wl_registry@2.bind(3, "wl_compositor", 3, new id [unknown]@4)
[3395258.433] wl_registry@2.global(4, "wl_subcompositor", 1)
WAYLAND INTERFACE: wl_subcompositor
[3395258.545] wl_registry@2.global(5, "wl_seat", 6)
WAYLAND INTERFACE: wl_seat
[3395258.645]  -> wl_registry@2.bind(5, "wl_seat", 5, new id [unknown]@5)
[3395258.848] wl_registry@2.global(6, "wl_output", 3)
WAYLAND INTERFACE: wl_output
[3395259.000]  -> wl_registry@2.bind(6, "wl_output", 2, new id [unknown]@6)
[3395259.112] wl_registry@2.global(7, "wl_data_device_manager", 3)
WAYLAND INTERFACE: wl_data_device_manager
[3395259.199]  -> wl_registry@2.bind(7, "wl_data_device_manager", 3, new id [unknown]@7)
[3395259.339] wl_registry@2.global(8, "wl_shell", 1)
WAYLAND INTERFACE: wl_shell
[3395259.437]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@8)
[3395259.598] wl_registry@2.global(9, "zxdg_shell_v6", 1)
WAYLAND INTERFACE: zxdg_shell_v6
[3395259.729]  -> wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@9)
[3395259.928] wl_registry@2.global(10, "xdg_wm_base", 1)
WAYLAND INTERFACE: xdg_wm_base
[3395260.023] wl_registry@2.global(11, "wl_shm", 1)
WAYLAND INTERFACE: wl_shm
[3395260.108]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@10)
[3395260.615]  -> wl_shm@10.create_pool(new id wl_shm_pool@11, fd 5, 4096)
[3395262.619]  -> wl_shm_pool@11.resize(12288)
[3395263.695]  -> wl_shm_pool@11.resize(28672)
[3395265.468]  -> wl_shm_pool@11.resize(61440)
[3395276.673]  -> wl_shm_pool@11.resize(126976)
[3395277.196]  -> wl_shm_pool@11.resize(258048)
[3395284.160]  -> wl_shm_pool@11.resize(520192)
[3395300.514]  -> wl_shm_pool@11.resize(1044480)
[3395348.549] wl_callback@3.done(255)
[3395348.697]  -> wl_display@1.sync(new id wl_callback@3)
[3395348.869] wl_seat@5.capabilities(7)
[3395348.920]  -> wl_seat@5.get_pointer(new id wl_pointer@12)
[3395349.019]  -> wl_seat@5.get_touch(new id wl_touch@13)
[3395349.071]  -> wl_seat@5.get_keyboard(new id wl_keyboard@14)
[3395349.117] wl_seat@5.name("seat0")
[3395349.862] wl_display@1.delete_id(3)
[3395349.959] wl_output@6.geometry(0, 0, 68, 136, 0, "Fake manufacturer", "Fake model", 0)
[3395350.071] wl_output@6.mode(3, 720, 1440, 553)
[3395350.213] wl_output@6.scale(1)
[3395350.256] wl_output@6.done()
[3395350.285] wl_callback@3.done(255)
[3395350.362]  -> wl_shm_pool@11.create_buffer(new id wl_buffer@3, 770048, 32, 32, 128, 0)
[3395350.475]  -> wl_compositor@4.create_surface(new id wl_surface@15)
[3395350.536]  -> wl_pointer@12.set_cursor(0, wl_surface@15, 10, 5)
[3395350.583]  -> wl_surface@15.attach(wl_buffer@3, 0, 0)
[3395350.616]  -> wl_surface@15.damage(0, 0, 32, 32)
[3395350.653]  -> wl_surface@15.commit()
Creating window...
[3395355.709]  -> wl_compositor@4.create_surface(new id wl_surface@16)
[3395355.813]  -> zxdg_shell_v6@9.get_xdg_surface(new id zxdg_surface_v6@17, wl_surface@16)
[3395355.853]  -> zxdg_surface_v6@17.get_toplevel(new id zxdg_toplevel_v6@18)
[3395355.882]  -> zxdg_toplevel_v6@18.set_app_id("filemanager.ubuntu.com.filemanager")
[3395355.945]  -> zxdg_toplevel_v6@18.destroy()
[3395355.982]  -> zxdg_surface_v6@17.destroy()
[3395356.009]  -> wl_surface@16.destroy()
filemanager.ubuntu.com.filemanager: sdl.c:24: main: Assertion `window != NULL' failed.

Thread 1 "filemanager.ubu" received signal SIGABRT, Aborted.
0x0000fffff7d5f568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
#0  0x0000fffff7d5f568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
#1  0x0000fffff7d60a20 in abort () from /lib/aarch64-linux-gnu/libc.so.6
#2  0x0000fffff7d58c44 in ?? () from /lib/aarch64-linux-gnu/libc.so.6
#3  0x0000000000400e40 in ?? ()
---Type <return> to continue, or q <return> to quit---Backtrace stopped: previous frame inner to this frame (corrupt stack?)
#0  0x0000fffff7d5f568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
(gdb) quit
*/
