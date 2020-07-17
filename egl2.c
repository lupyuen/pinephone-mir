//  EGL Wayland App that renders a bitmap on PinePhone with Ubuntu Touch.
//  To build and run on PinePhone, see egl2.sh.
//  Bundled source files: texture.c, util.c, util.h
//  Sample log: logs/egl2.log 
//  Based on https://jan.newmarch.name/Wayland/EGL/
//  and https://jan.newmarch.name/Wayland/WhenCanIDraw/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "util.h"

static void shm_format(void *data, struct wl_shm *wl_shm, uint32_t format);
void Init ( ESContext *esContext );
void Draw ( ESContext *esContext );

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;
struct wl_shm *shm;
struct wl_buffer *buffer;
struct wl_callback *frame_callback;
void *shm_data;

struct wl_shm_listener shm_listener = {
    shm_format
};

int WIDTH = 480;
int HEIGHT = 360;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

////////////////////////////////////////////////////////////////////
//  EGL

/// Render the GLES2 display
void render_display()
{
    puts("Rendering display...");
    // glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Set background color to magenta and opaque
    // glClear(GL_COLOR_BUFFER_BIT);         // Clear the color buffer (background)

    ////
    static ESContext esContext;
    esInitContext ( &esContext );
    esContext.width = WIDTH;
    esContext.height = HEIGHT;

    Init(&esContext);
    Draw(&esContext);
    ////

    glFlush(); // Render now
}

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                        const char *interface, uint32_t version)
{
    printf("Got a registry event for %s id %d\n", interface, id);
    if (strcmp(interface, "wl_compositor") == 0)
    {
        compositor = wl_registry_bind(registry,
                                      id,
                                      &wl_compositor_interface,
                                      1);
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 1);
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        shm = wl_registry_bind(registry, id,
                               &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover};

static void
create_opaque_region()
{
    puts("Creating opaque region...");
    region = wl_compositor_create_region(compositor);
    assert(region != NULL);

    wl_region_add(region, 0, 0,
                  WIDTH,
                  HEIGHT);
    wl_surface_set_opaque_region(surface, region);
}

static void
create_window()
{
    puts("Creating window...");
    egl_window = wl_egl_window_create(surface,
                                      WIDTH, HEIGHT);
    if (egl_window == EGL_NO_SURFACE)
    {
        fprintf(stderr, "Can't create egl window\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Created egl window\n");
    }

    egl_surface =
        eglCreateWindowSurface(egl_display,
                               egl_conf,
                               egl_window, NULL);

    if (eglMakeCurrent(egl_display, egl_surface,
                       egl_surface, egl_context))
    {
        fprintf(stderr, "Made current\n");
    }
    else
    {
        fprintf(stderr, "Made current failed\n");
    }

    // Render the display
    render_display();

    if (eglSwapBuffers(egl_display, egl_surface))
    {
        fprintf(stderr, "Swapped buffers\n");
    }
    else
    {
        fprintf(stderr, "Swapped buffers failed\n");
    }
}

static void
init_egl()
{
    puts("Init EGL...");
    EGLint major, minor, count, n, size;
    EGLConfig *configs;
    int i;
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE};

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE};

    egl_display = eglGetDisplay((EGLNativeDisplayType)display);
    if (egl_display == EGL_NO_DISPLAY)
    {
        fprintf(stderr, "Can't create egl display\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Created egl display\n");
    }

    if (eglInitialize(egl_display, &major, &minor) != EGL_TRUE)
    {
        fprintf(stderr, "Can't initialise egl display\n");
        exit(1);
    }
    printf("EGL major: %d, minor %d\n", major, minor);

    eglGetConfigs(egl_display, NULL, 0, &count);
    printf("EGL has %d configs\n", count);

    configs = calloc(count, sizeof *configs);

    eglChooseConfig(egl_display, config_attribs,
                    configs, count, &n);

    for (i = 0; i < n; i++)
    {
        eglGetConfigAttrib(egl_display,
                           configs[i], EGL_BUFFER_SIZE, &size);
        printf("Buffer size for config %d is %d\n", i, size);
        eglGetConfigAttrib(egl_display,
                           configs[i], EGL_RED_SIZE, &size);
        printf("Red size for config %d is %d\n", i, size);

        // just choose the first one
        egl_conf = configs[i];
        break;
    }

    egl_context =
        eglCreateContext(egl_display,
                         egl_conf,
                         EGL_NO_CONTEXT, context_attribs);
}

static void
get_server_references(void)
{
    puts("Getting server references...");
    display = wl_display_connect(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    printf("connected to display\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    assert(registry != NULL);

    wl_registry_add_listener(registry, &registry_listener, NULL);

    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL || shell == NULL)
    {
        fprintf(stderr, "Can't find compositor or shell\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Found compositor and shell\n");
    }
}

////////////////////////////////////////////////////////////////////
//  Shared Memory

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
            uint32_t serial)
{
    puts("Handling ping...");
    wl_shell_surface_pong(shell_surface, serial);
    fprintf(stderr, "Pinged and ponged\n");
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
    printf("Handling configure: edges=%d, width=%d, height=%d\n", edges, width, height);
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
    puts("Handling popup done");
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done};

static int
set_cloexec_or_close(int fd)
{
    long flags;

    if (fd == -1)
        return -1;

    flags = fcntl(fd, F_GETFD);
    if (flags == -1)
        goto err;

    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == -1)
        goto err;

    return fd;

err:
    close(fd);
    return -1;
}

static int
create_tmpfile_cloexec(char *tmpname)
{
    int fd;

#ifdef HAVE_MKOSTEMP
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0)
        unlink(tmpname);
#else
    fd = mkstemp(tmpname);
    if (fd >= 0)
    {
        fd = set_cloexec_or_close(fd);
        unlink(tmpname);
    }
#endif

    return fd;
}

/*
 * Create a new, unique, anonymous file of the given size, and
 * return the file descriptor for it. The file descriptor is set
 * CLOEXEC. The file is immediately suitable for mmap()'ing
 * the given size at offset zero.
 *
 * The file should not have a permanent backing store like a disk,
 * but may have if XDG_RUNTIME_DIR is not properly implemented in OS.
 *
 * The file name is deleted from the file system.
 *
 * The file is suitable for buffer sharing between processes by
 * transmitting the file descriptor over Unix sockets using the
 * SCM_RIGHTS methods.
 */
int os_create_anonymous_file(off_t size)
{
    static const char template[] = "/weston-shared-XXXXXX";
    const char *path;
    char *name;
    int fd;

    path = getenv("XDG_RUNTIME_DIR");
    if (!path)
    {
        errno = ENOENT;
        return -1;
    }

    name = malloc(strlen(path) + sizeof(template));
    if (!name)
        return -1;
    strcpy(name, path);
    strcat(name, template);

    printf("Creating anonymous file %s...\n", name);
    fd = create_tmpfile_cloexec(name);

    free(name);

    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0)
    {
        close(fd);
        return -1;
    }

    return fd;
}

uint32_t pixel_value = 0x0; // black

static void
paint_pixels()
{
    puts("Painting...");
    int n;
    uint32_t *pixel = shm_data;
    assert(pixel != NULL);

    for (n = 0; n < WIDTH * HEIGHT; n++)
    {
        *pixel++ = pixel_value;
    }

    // increase each RGB component by one
    pixel_value += 0x10101;

    // if it's reached 0xffffff (white) reset to zero
    if (pixel_value > 0xffffff)
    {
        pixel_value = 0x0;
    }
}

static const struct wl_callback_listener frame_listener;

int ht;

static void
redraw(void *data, struct wl_callback *callback, uint32_t time)
{
    puts("Redrawing...");
    assert(surface != NULL);
    wl_callback_destroy(frame_callback);
    if (ht == 0)
        ht = HEIGHT;
    wl_surface_damage(surface, 0, 0,
                      WIDTH, ht--);
    paint_pixels();
    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);
    wl_surface_commit(surface);
}

static const struct wl_callback_listener frame_listener = {
    redraw
};

static struct wl_buffer *
create_buffer()
{
    puts("Creating buffer...");
    struct wl_shm_pool *pool;
    int stride = WIDTH * 4; // 4 bytes per pixel
    int size = stride * HEIGHT;
    int fd;
    struct wl_buffer *buff;

    ht = HEIGHT;

    fd = os_create_anonymous_file(size);
    if (fd < 0)
    {
        fprintf(stderr, "creating a buffer file for %d B failed: %m\n",
                size);
        exit(1);
    }

    shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (shm_data == MAP_FAILED)
    {
        fprintf(stderr, "mmap failed: %m\n");
        close(fd);
        exit(1);
    }

    assert(shm != NULL);
    pool = wl_shm_create_pool(shm, fd, size);
    assert(pool != NULL);

    buff = wl_shm_pool_create_buffer(pool, 0,
                                     WIDTH, HEIGHT,
                                     stride,
                                     WL_SHM_FORMAT_XRGB8888);
    //wl_buffer_add_listener(buffer, &buffer_listener, buffer);
    wl_shm_pool_destroy(pool);
    return buff;
}

static void
create_window_shared_memory()
{
    puts("Creating window with shared memory...");
    buffer = create_buffer();
    assert(buffer != NULL);

    assert(surface != NULL);
    wl_surface_attach(surface, buffer, 0, 0);
    //wl_surface_damage(surface, 0, 0, WIDTH, HEIGHT);
    wl_surface_commit(surface);
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    char *s;
    switch (format)
    {
    case WL_SHM_FORMAT_ARGB8888:
        s = "ARGB8888";
        break;
    case WL_SHM_FORMAT_XRGB8888:
        s = "XRGB8888";
        break;
    case WL_SHM_FORMAT_RGB565:
        s = "RGB565";
        break;
    default:
        s = "other format";
        break;
    }
    fprintf(stderr, "Possible shmem format %s\n", s);
}

////////////////////////////////////////////////////////////////////
//  Main

int main(int argc, char **argv)
{
    get_server_references();

    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL)
    {
        fprintf(stderr, "Can't create surface\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Created surface\n");
    }

    shell_surface = wl_shell_get_shell_surface(shell, surface);
    assert(shell_surface != NULL);

    wl_shell_surface_set_toplevel(shell_surface);

#ifdef USE_SHARED_MEMORY
    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);
#endif  //  USE_SHARED_MEMORY    

    create_opaque_region();
    init_egl();

#ifdef USE_SHARED_MEMORY
    create_window_shared_memory();
    redraw(NULL, NULL, 0);
#else
    create_window();
#endif  //  USE_SHARED_MEMORY    

    while (wl_display_dispatch(display) != -1)
    {
        ;
    }

    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}

#ifdef NOTUSED
Output with Shared Memory:

++ gcc -g -o egl2 egl2.c -lwayland-client -lwayland-server -lwayland-egl -L/usr/lib/aarch64-linux-gnu/mesa-egl -lEGL /usr/lib/aarch64-linux-gnu/mesa-egl/libGLESv2.so.2 -Wl,-Map=egl2.map
++ sudo mount -o remount,rw /
++ sudo cp egl2 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl2
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl2
-rwxr-xr-x 1 clickpkg clickpkg 40944 Jul 15 16:04 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl2
++ sudo cp run.sh /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ echo '*** Tap on File Manager icon on PinePhone'
*** Tap on File Manager icon on PinePhone
++ echo
++ tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log

GNU gdb (Ubuntu 7.11.1-0ubuntu1~16.5) 7.11.1
Copyright (C) 2016 Free Software Foundation, Inc.
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>
This is free software: you are free to change and redistribute it.
There is NO WARRANTY, to the extent permitted by law.  Type "show copying"
and "show warranty" for details.
This GDB was configured as "aarch64-linux-gnu".
Type "show configuration" for configuration details.
For bug reporting instructions, please see:
<http://www.gnu.org/software/gdb/bugs/>.
Find the GDB manual and other documentation resources online at:
<http://www.gnu.org/software/gdb/documentation/>.
For help, type "help".
Type "apropos word" to search for commands related to "word"...
Reading symbols from ./egl2...done.
Starting program: /usr/share/click/preinstalled/com.ubuntu.filemanager/0.7.5/egl2 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
Getting server references...
connected to display
[1609874.744]  -> wl_display@1.get_registry(new id wl_registry@2)
[1609892.217] wl_registry@2.global(1, "wl_drm", 2)
Got a registry event for wl_drm id 1
[1609892.472] wl_registry@2.global(2, "qt_windowmanager", 1)
Got a registry event for qt_windowmanager id 2
[1609892.563] wl_registry@2.global(3, "wl_compositor", 4)
Got a registry event for wl_compositor id 3
[1609892.682]  -> wl_registry@2.bind(3, "wl_compositor", 1, new id [unknown]@3)
[1609892.800] wl_registry@2.global(4, "wl_subcompositor", 1)
Got a registry event for wl_subcompositor id 4
[1609892.899] wl_registry@2.global(5, "wl_seat", 6)
Got a registry event for wl_seat id 5
[1609892.969] wl_registry@2.global(6, "wl_output", 3)
Got a registry event for wl_output id 6
[1609893.045] wl_registry@2.global(7, "wl_data_device_manager", 3)
Got a registry event for wl_data_device_manager id 7
[1609893.099] wl_registry@2.global(8, "wl_shell", 1)
Got a registry event for wl_shell id 8
[1609893.213]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@4)
[1609893.328] wl_registry@2.global(9, "zxdg_shell_v6", 1)
Got a registry event for zxdg_shell_v6 id 9
[1609893.427] wl_registry@2.global(10, "xdg_wm_base", 1)
Got a registry event for xdg_wm_base id 10
[1609893.519] wl_registry@2.global(11, "wl_shm", 1)
Got a registry event for wl_shm id 11
[1609893.617]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@5)
[1609893.725]  -> wl_display@1.sync(new id wl_callback@6)
[1609894.123] wl_display@1.delete_id(6)
[1609894.199] wl_shm@5.format(0)
Possible shmem format ARGB8888
[1609894.278] wl_shm@5.format(1)
Possible shmem format XRGB8888
[1609894.338] wl_callback@6.done(31)
Found compositor and shell
[1609894.406]  -> wl_compositor@3.create_surface(new id wl_surface@6)
Created surface
[1609894.463]  -> wl_shell@4.get_shell_surface(new id wl_shell_surface@7, wl_surface@6)
[1609894.538]  -> wl_shell_surface@7.set_toplevel()
[1609894.574]  -> wl_surface@6.frame(new id wl_callback@8)
Creating opague region...
[1609894.642]  -> wl_compositor@3.create_region(new id wl_region@9)
[1609894.681]  -> wl_region@9.add(0, 0, 480, 360)
[1609894.782]  -> wl_surface@6.set_opaque_region(wl_region@9)
Init EGL...
Created egl display
[1609895.320]  -> wl_display@1.get_registry(new id wl_registry@10)
[1609895.412]  -> wl_display@1.sync(new id wl_callback@11)
[1609895.918] wl_display@1.delete_id(11)
[1609895.996] wl_registry@10.global(1, "wl_drm", 2)
[1609896.104]  -> wl_registry@10.bind(1, "wl_drm", 2, new id [unknown]@12)
[1609896.332] wl_registry@10.global(2, "qt_windowmanager", 1)
[1609896.406] wl_registry@10.global(3, "wl_compositor", 4)
[1609896.560] wl_registry@10.global(4, "wl_subcompositor", 1)
[1609896.665] wl_registry@10.global(5, "wl_seat", 6)
[1609896.754] wl_registry@10.global(6, "wl_output", 3)
[1609896.833] wl_registry@10.global(7, "wl_data_device_manager", 3)
[1609896.911] wl_registry@10.global(8, "wl_shell", 1)
[1609896.976] wl_registry@10.global(9, "zxdg_shell_v6", 1)
[1609897.050] wl_registry@10.global(10, "xdg_wm_base", 1)
[1609897.104] wl_registry@10.global(11, "wl_shm", 1)
[1609897.177] wl_callback@11.done(31)
[1609897.238]  -> wl_display@1.sync(new id wl_callback@11)
[1609897.999] wl_display@1.delete_id(11)
[1609898.072] wl_drm@12.device("/dev/dri/card1")
[1609898.434]  -> wl_drm@12.authenticate(4)
[1609898.493] wl_drm@12.format(875713089)
[1609898.543] wl_drm@12.format(875713112)
[1609898.588] wl_drm@12.format(909199186)
[1609898.650] wl_drm@12.format(961959257)
[1609898.698] wl_drm@12.format(825316697)
[1609898.732] wl_drm@12.format(842093913)
[1609898.777] wl_drm@12.format(909202777)
[1609898.817] wl_drm@12.format(875713881)
[1609898.857] wl_drm@12.format(842094158)
[1609898.902] wl_drm@12.format(909203022)
[1609898.938] wl_drm@12.format(1448695129)
[1609898.972] wl_drm@12.capabilities(1)
[1609899.016] wl_callback@11.done(31)
[1609899.073]  -> wl_display@1.sync(new id wl_callback@11)
[1609899.747] wl_display@1.delete_id(11)
[1609901.542] wl_drm@12.authenticated()
[1609903.286] wl_callback@11.done(31)
EGL major: 1, minor 4
EGL has 14 configs
Buffer size for config 0 is 24
Red size for config 0 is 8
Creating window with shared memory...
Creating buffer...
Creating anonymous file /run/user/32011/weston-shared-XXXXXX...
[1610345.063]  -> wl_shm@5.create_pool(new id wl_shm_pool@11, fd 9, 1024)
[1610345.189]  -> wl_shm_pool@11.create_buffer(new id wl_buffer@13, 0, 16, 16, 64, 1)
[1610345.295]  -> wl_shm_pool@11.destroy()
[1610345.332]  -> wl_surface@6.attach(wl_buffer@13, 0, 0)
[1610345.395]  -> wl_surface@6.commit()
Redrawing...
[1610345.438]  -> wl_surface@6.damage(0, 0, 16, 16)
Painting...
[1610345.624]  -> wl_surface@6.frame(new id wl_callback@14)
[1610345.664]  -> wl_surface@6.attach(wl_buffer@13, 0, 0)
[1610345.733]  -> wl_surface@6.commit()
[1610352.605] wl_display@1.delete_id(11)
disconnected from display
[Inferior 1 (process 29077) exited normally]
No stack.
No stack.
(gdb) quit
Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files
#endif  //  NOTUSED

#ifdef NOTUSED
Output without Shared Memory:

Getting server references...
connected to display
[2310849.806]  -> wl_display@1.get_registry(new id wl_registry@2)
[2310869.547] wl_registry@2.global(1, "wl_drm", 2)
Got a registry event for wl_drm id 1
[2310869.755] wl_registry@2.global(2, "qt_windowmanager", 1)
Got a registry event for qt_windowmanager id 2
[2310869.859] wl_registry@2.global(3, "wl_compositor", 4)
Got a registry event for wl_compositor id 3
[2310869.969]  -> wl_registry@2.bind(3, "wl_compositor", 1, new id [unknown]@3)
[2310870.086] wl_registry@2.global(4, "wl_subcompositor", 1)
Got a registry event for wl_subcompositor id 4
[2310870.183] wl_registry@2.global(5, "wl_seat", 6)
Got a registry event for wl_seat id 5
[2310870.430] wl_registry@2.global(6, "wl_output", 3)
Got a registry event for wl_output id 6
[2310870.513] wl_registry@2.global(7, "wl_data_device_manager", 3)
Got a registry event for wl_data_device_manager id 7
[2310870.586] wl_registry@2.global(8, "wl_shell", 1)
Got a registry event for wl_shell id 8
[2310870.642]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@4)
[2310870.729] wl_registry@2.global(9, "zxdg_shell_v6", 1)
Got a registry event for zxdg_shell_v6 id 9
[2310870.823] wl_registry@2.global(10, "xdg_wm_base", 1)
Got a registry event for xdg_wm_base id 10
[2310871.008] wl_registry@2.global(11, "wl_shm", 1)
Got a registry event for wl_shm id 11
[2310871.104]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@5)
[2310871.216]  -> wl_display@1.sync(new id wl_callback@6)
[2310871.710] wl_display@1.delete_id(6)
[2310871.785] wl_shm@5.format(0)
Possible shmem format ARGB8888
[2310871.894] wl_shm@5.format(1)
Possible shmem format XRGB8888
[2310871.939] wl_callback@6.done(0)
Found compositor and shell
[2310872.006]  -> wl_compositor@3.create_surface(new id wl_surface@6)
Created surface
[2310872.073]  -> wl_shell@4.get_shell_surface(new id wl_shell_surface@7, wl_surface@6)
[2310872.155]  -> wl_shell_surface@7.set_toplevel()
Creating opaque region...
[2310872.202]  -> wl_compositor@3.create_region(new id wl_region@8)
[2310872.261]  -> wl_region@8.add(0, 0, 480, 360)
[2310872.329]  -> wl_surface@6.set_opaque_region(wl_region@8)
Init EGL...
Created egl display
[2310872.924]  -> wl_display@1.get_registry(new id wl_registry@9)
[2310873.018]  -> wl_display@1.sync(new id wl_callback@10)
[2310873.471] wl_display@1.delete_id(10)
[2310873.554] wl_registry@9.global(1, "wl_drm", 2)
[2310873.669]  -> wl_registry@9.bind(1, "wl_drm", 2, new id [unknown]@11)
[2310873.803] wl_registry@9.global(2, "qt_windowmanager", 1)
[2310873.887] wl_registry@9.global(3, "wl_compositor", 4)
[2310873.967] wl_registry@9.global(4, "wl_subcompositor", 1)
[2310874.044] wl_registry@9.global(5, "wl_seat", 6)
[2310874.121] wl_registry@9.global(6, "wl_output", 3)
[2310874.192] wl_registry@9.global(7, "wl_data_device_manager", 3)
[2310874.259] wl_registry@9.global(8, "wl_shell", 1)
[2310874.338] wl_registry@9.global(9, "zxdg_shell_v6", 1)
[2310874.389] wl_registry@9.global(10, "xdg_wm_base", 1)
[2310874.459] wl_registry@9.global(11, "wl_shm", 1)
[2310874.538] wl_callback@10.done(0)
[2310874.596]  -> wl_display@1.sync(new id wl_callback@10)
[2310878.802] wl_display@1.delete_id(10)
[2310878.870] wl_drm@11.device("/dev/dri/card1")
[2310879.276]  -> wl_drm@11.authenticate(3)
[2310879.369] wl_drm@11.format(875713089)
[2310879.398] wl_drm@11.format(875713112)
[2310879.419] wl_drm@11.format(909199186)
[2310879.439] wl_drm@11.format(961959257)
[2310879.459] wl_drm@11.format(825316697)
[2310879.480] wl_drm@11.format(842093913)
[2310879.500] wl_drm@11.format(909202777)
[2310879.520] wl_drm@11.format(875713881)
[2310879.539] wl_drm@11.format(842094158)
[2310879.559] wl_drm@11.format(909203022)
[2310879.578] wl_drm@11.format(1448695129)
[2310879.598] wl_drm@11.capabilities(1)
[2310879.618] wl_callback@10.done(0)
[2310879.650]  -> wl_display@1.sync(new id wl_callback@10)
[2310880.365] wl_display@1.delete_id(10)
[2310880.501] wl_drm@11.authenticated()
[2310880.534] wl_callback@10.done(0)
EGL major: 1, minor 4
EGL has 14 configs
Buffer size for config 0 is 24
Red size for config 0 is 8
Creating window...
Created egl window
Made current
Rendering display...
[2311326.953]  -> wl_surface@6.frame(new id wl_callback@10)
[2311327.259]  -> wl_drm@11.create_prime_buffer(new id wl_buffer@12, fd 9, 480, 360, 875713112, 0, 1920, 0, 0, 0, 0)
[2311327.486]  -> wl_surface@6.attach(wl_buffer@12, 0, 0)
[2311327.545]  -> wl_surface@6.damage(0, 0, 2147483647, 2147483647)
[2311327.643]  -> wl_surface@6.commit()
Swapped buffers
[2311469.045] wl_display@1.delete_id(10)
#endif  //  NOTUSED