//  EGL Wayland App that renders a bitmap on PinePhone with Ubuntu Touch.
//  To build and run on PinePhone, see egl2.sh.
//  Bundled source files: shader.c, texture.c, util.c, util.h
//  Sample log: logs/egl2.log 
//  Based on https://jan.newmarch.name/Wayland/EGL/
//  and https://jan.newmarch.name/Wayland/WhenCanIDraw/
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
#include "util.h"

/// Local Functions
static void get_server_references(void);
static void create_opaque_region(void);
static void create_window(void);
static void init_egl(void);

/// Dimensions of the OpenGL region to be rendered
static int WIDTH  = 480;
static int HEIGHT = 360;

/// Wayland Interfaces
static struct wl_display       *display;       //  Wayland Display
static struct wl_compositor    *compositor;    //  Wayland Compositor
static struct wl_surface       *surface;       //  Wayland Surface
static struct wl_egl_window    *egl_window;    //  Wayland EGL Window
static struct wl_region        *region;        //  Wayland Region
static struct wl_shell         *shell;         //  Wayland Shell
static struct wl_shell_surface *shell_surface; //  Wayland Shell Surface

/// Wayland EGL Interfaces for OpenGL Rendering
static EGLDisplay egl_display;  //  EGL Display
static EGLConfig  egl_conf;     //  EGL Configuration
static EGLSurface egl_surface;  //  EGL Surface
static EGLContext egl_context;  //  EGL Context

////////////////////////////////////////////////////////////////////
//  Render OpenGL

/// Exported by texture.c
void Init(ESContext *esContext);
void Draw(ESContext *esContext);

/// Render the OpenGL ES2 display
static void render_display() {
    puts("Rendering display...");

    //  Create the texture context
    static ESContext esContext;
    esInitContext ( &esContext );
    esContext.width = WIDTH;
    esContext.height = HEIGHT;

    //   Draw the texture
    Init(&esContext);
    Draw(&esContext);

    //  Render now
    glFlush();
}


////////////////////////////////////////////////////////////////////
//  Main

/// Connect to Wayland Compositor and render OpenGL graphics
int main(int argc, char **argv) {
    //  Get interfaces for Wayland Compositor and Wayland Shell
    get_server_references();
    assert(compositor != NULL);  //  Failed to get Wayland Compositor
    assert(shell != NULL);       //  Failed to get Wayland Shell

    //  Create a Wayland Surface for rendering
    surface = wl_compositor_create_surface(compositor);
    assert(surface != NULL);  //  Failed to create Wayland Surface

    //  Get the Wayland Shell Surface for rendering
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    assert(shell_surface != NULL);

    //  Set the Shell Surface as top level
    wl_shell_surface_set_toplevel(shell_surface);

    //  Create an opaque region for rendering
    create_opaque_region();

    //  Init Wayland EGL
    init_egl();

    //  Create the OpenGL Window and render OpenGL graphics
    create_window();

    //  Handle all Wayland Events in the Event Loop
    while (wl_display_dispatch(display) != -1) {}

    //  Disconnect from the Wayland Display
    wl_display_disconnect(display);
    puts("Disconnected from display");
    return 0;
}

////////////////////////////////////////////////////////////////////
//  Wayland Registry

static void global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
    const char *interface, uint32_t version);
static void global_registry_remover(void *data, struct wl_registry *registry, uint32_t id);

/// Callbacks for interfaces returned by Wayland Service
static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

/// Connect to Wayland Service and fetch the interfaces for Wayland Compositor and Wayland Shell
static void get_server_references(void) {
    puts("Getting server references...");
    display = wl_display_connect(NULL);
    if (display == NULL) {
        fprintf(stderr, "Failed to connect to display\n");
        exit(1);
    }
    puts("Connected to display");

    //  Get the Wayland Registry
    struct wl_registry *registry = wl_display_get_registry(display);
    assert(registry != NULL);  //  Failed to get Wayland Registry

    //  Add Registry Callbacks to handle interfaces returned by Wayland Service
    wl_registry_add_listener(registry, &registry_listener, NULL);

    //  Wait for Registry Callbacks to fetch Wayland Interfaces
    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    //  We should have received interfaces for Wayland Compositor and Wayland Shell
    assert(compositor != NULL);  //  Failed to get Wayland Compositor
    assert(shell != NULL);       //  Failed to get Wayland Shell
}

/// Callback for interfaces returned by Wayland Service
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
    printf("Removed interface id %d\n", id);
}

////////////////////////////////////////////////////////////////////
//  Wayland EGL

/// Create an opaque region for OpenGL rendering
static void create_opaque_region(void) {
    puts("Creating opaque region...");
    region = wl_compositor_create_region(compositor);
    assert(region != NULL);  //  Failed to create EGL Region

    wl_region_add(region, 0, 0, WIDTH, HEIGHT);
    wl_surface_set_opaque_region(surface, region);
}

/// Create the OpenGL window and render it
static void create_window(void) {
    //  Create an OpenGL Window
    egl_window = wl_egl_window_create(surface, WIDTH, HEIGHT);
    assert(egl_window != EGL_NO_SURFACE);  //  Failed to create OpenGL Window

    //  Create an OpenGL Window Surface for rendering
    egl_surface = eglCreateWindowSurface(egl_display, egl_conf,
        egl_window, NULL);
    assert(egl_surface != NULL);  //  Failed to create OpenGL Window Surface

    //  Set the current rendering surface
    EGLBoolean madeCurrent = eglMakeCurrent(egl_display, egl_surface,
        egl_surface, egl_context);
    assert(madeCurrent);  //  Failed to set rendering surface

    //  Render the display
    render_display();

    //  Swap the display buffers to make the display visible
    EGLBoolean swappedBuffers = eglSwapBuffers(egl_display, egl_surface);
    assert(swappedBuffers);  //  Failed to swap display buffers
}

/// Init the EGL Interface
static void init_egl(void) {
    puts("Init EGL...");

    //  Attributes for our EGL Display
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,        8,
        EGL_GREEN_SIZE,      8,
        EGL_BLUE_SIZE,       8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    //  Get the EGL Display
    egl_display = eglGetDisplay((EGLNativeDisplayType) display);
    assert(egl_display != EGL_NO_DISPLAY);  //  Failed to get EGL Display

    //  Init the EGL Display
    EGLint major, minor;
    EGLBoolean egl_init = eglInitialize(egl_display, &major, &minor);
    assert(egl_init);  //  Failed to init EGL Display
    printf("EGL major: %d, minor %d\n", major, minor);

    //  Get the EGL Configurations
    EGLint count, n;
    eglGetConfigs(egl_display, NULL, 0, &count);
    printf("EGL has %d configs\n", count);
    EGLConfig *configs = calloc(count, sizeof *configs);
    eglChooseConfig(egl_display, config_attribs,
        configs, count, &n);

    //  Choose the first EGL Configuration
    for (int i = 0; i < n; i++) {
        EGLint size;
        eglGetConfigAttrib(egl_display, configs[i], EGL_BUFFER_SIZE, &size);
        printf("Buffer size for config %d is %d\n", i, size);
        eglGetConfigAttrib(egl_display, configs[i], EGL_RED_SIZE, &size);
        printf("Red size for config %d is %d\n", i, size);
        egl_conf = configs[i];
        break;
    }
    assert(egl_conf != NULL);  //  Failed to get EGL Configuration

    //  Create the EGL Context based on the EGL Display and Configuration
    egl_context = eglCreateContext(egl_display, egl_conf,
        EGL_NO_CONTEXT, context_attribs);
    assert(egl_context != NULL);  //  Failed to create EGL Context
}
