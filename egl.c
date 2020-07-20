//  Simple EGL Wayland App that draws a yellow rectangle on PinePhone with Ubuntu Touch
//  To build and run on PinePhone, see egl.sh.
//  Sample log: logs/egl.log 
//  Based on https://jan.newmarch.name/Wayland/EGL/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
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

/// Dimensions of the OpenGL region to be rendered
int WIDTH  = 480;
int HEIGHT = 360;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

/// Render the OpenGL ES2 display
void render_display() {
    puts("Rendering display...");

    //  Fill the rectangular region with yellow
    glClearColor(
        1.0,  //  Red
        1.0,  //  Green
        0.0,  //  Blue
        1.0   //  Alpha
    );
    glClear(GL_COLOR_BUFFER_BIT);

    // Render now
    glFlush();
}

/// Callback for interfaces returned by Wayland Compositor
static void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t version) {
    printf("Got interface %s id %d\n", interface, id);

    if (strcmp(interface, "wl_compositor") == 0) {
        //  Bind to Wayland Compositor Interface
        compositor = wl_registry_bind(registry, id,
            &wl_compositor_interface,   //  Interface Type
            1);                         //  Interface Version
    } else if (strcmp(interface, "wl_shell") == 0){
        //  Bind to Wayland Shell Interface
        shell = wl_registry_bind(registry, id,
            &wl_shell_interface,        //  Interface Type
            1);                         //  Interface Version
    }
}

/// Callback for removed interfaces
static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id) {
    printf("Removed id %d\n", id);
}

/// Callbacks for interfaces returned by Wayland Compositor
static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

/// Create an opaque region for OpenGL rendering
static void create_opaque_region() {
    puts("Creating opaque region...");
    region = wl_compositor_create_region(compositor);
    assert(region != NULL);

    wl_region_add(region, 0, 0, WIDTH, HEIGHT);
    wl_surface_set_opaque_region(surface, region);
}

/// Create the OpenGL window and render it
static void create_window() {
    //  Create an OpenGL Window
    egl_window = wl_egl_window_create(surface, WIDTH, HEIGHT);
    assert(egl_window != EGL_NO_SURFACE);

    //  Create an OpenGL Window Surface for rendering
    egl_surface = eglCreateWindowSurface(egl_display, egl_conf,
        egl_window, NULL);
    assert(egl_surface != NULL);

    //  Set the current rendering surface
    EGLBoolean madeCurrent = eglMakeCurrent(egl_display, egl_surface,
        egl_surface, egl_context);
    assert(madeCurrent);

    //  Render the display
    render_display();

    //  Swap the display buffers to make the display visible
    EGLBoolean swappedBuffers = eglSwapBuffers(egl_display, egl_surface);
    assert(swappedBuffers);
}

static void
init_egl()
{
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

    display = wl_display_connect(NULL);
    if (display == NULL)
    {
        fprintf(stderr, "Can't connect to display\n");
        exit(1);
    }
    printf("connected to display\n");

    struct wl_registry *registry = wl_display_get_registry(display);
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
    wl_shell_surface_set_toplevel(shell_surface);

    create_opaque_region();
    init_egl();
    create_window();

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