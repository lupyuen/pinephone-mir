//  EGL Wayland App that renders simple graphics on PinePhone with Ubuntu Touch.
//  To build and run on PinePhone, see egl2.sh.
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

int WIDTH = 16;
int HEIGHT = 16;

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
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Set background color to magenta and opaque
    glClear(GL_COLOR_BUFFER_BIT);         // Clear the color buffer (background)

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
    puts("Creating opague region...");
    region = wl_compositor_create_region(compositor);
    assert(region != NULL);

    wl_region_add(region, 0, 0,
                  480,
                  360);
    wl_surface_set_opaque_region(surface, region);
}

static void
create_window()
{
    puts("Creating window...");
    egl_window = wl_egl_window_create(surface,
                                      480, 360);
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

    pool = wl_shm_create_pool(shm, fd, size);
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

struct wl_shm_listener shm_listener = {
    shm_format};

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

    ////
    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);
    ////

    create_opaque_region();
    init_egl();
    ////  create_window();
    create_window_shared_memory();

    ////
    redraw(NULL, NULL, 0);
    ////

    while (wl_display_dispatch(display) != -1)
    {
        ;
    }

    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}

/* Output:
++ gcc -o egl egl.c -lwayland-client -lwayland-server -lwayland-egl -L/usr/lib/aarch64-linux-gnu/mesa-egl -lEGL /usr/lib/aarch64-linux-gnu/mesa-egl/libGLESv2.so.2 -Wl,-Map=egl.map
++ sudo mount -o remount,rw /
++ sudo cp egl /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
-rwxr-xr-x 1 clickpkg clickpkg 20568 Jul 12 03:27 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
++ echo
++ tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log

connected to display
Got a registry event for wl_drm id 1
Got a registry event for qt_windowmanager id 2
Got a registry event for wl_compositor id 3
Got a registry event for wl_subcompositor id 4
Got a registry event for wl_seat id 5
Got a registry event for wl_output id 6
Got a registry event for wl_data_device_manager id 7
Got a registry event for wl_shell id 8
Got a registry event for zxdg_shell_v6 id 9
Got a registry event for xdg_wm_base id 10
Got a registry event for wl_shm id 11
Found compositor and shell
Created surface
Created egl display
EGL major: 1, minor 4
EGL has 14 configs
Buffer size for config 0 is 24
Red size for config 0 is 8
Created egl window
Made current
Swapped buffers
*/