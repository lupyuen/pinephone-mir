//  Wayland App that uses Shared Memory to render graphics on PinePhone with Ubuntu Touch.
//  Note: This program fails with error "wl_drm@4: error 2: invalid name"
//  To build and run on PinePhone, see shm.sh.
//  For output log, see logs/shm.log
//  Based on inspection of Ubuntu Touch File Manager Wayland Log (logs/filemanager-wayland.log)
//  and https://jan.newmarch.name/Wayland/WhenCanIDraw/
//  and https://bugaevc.gitbooks.io/writing-wayland-clients/beyond-the-black-square/xdg-shell.html
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syscall.h>
#include <sys/mman.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-server-protocol.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include "xdg-shell.h"
#include "wayland-drm-client-protocol.h"

extern char **environ;
int fallocate(int fd, int mode, off_t offset, off_t len);

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_egl_window *egl_window;
struct wl_region *region;
struct wl_shm *shm;
struct wl_drm *drm;
struct wl_buffer *buffer;
struct wl_callback *frame_callback;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;

struct zxdg_shell_v6 *zshell;
struct zxdg_surface_v6 *zsurface;
struct zxdg_toplevel_v6 *toplevel;

EGLDisplay egl_display;
EGLConfig egl_conf;
EGLSurface egl_surface;
EGLContext egl_context;

int pool_fd = -1;
void *shm_data;

//  Note: If height or width are below 160, wl_drm will fail with error "invalid name"
//  https://github.com/alacritty/alacritty/issues/2895
int WIDTH = 240;
int HEIGHT = 240;

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
            uint32_t serial)
{
    puts("Handling ping...");
    assert(shell_surface != NULL);
    wl_shell_surface_pong(shell_surface, serial);
    fprintf(stderr, "Pinged and ponged\n");
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
    printf("Handling configure: edges=%d, width=%d, height=%d\n", edges, width, height);
    assert(shell_surface != NULL);
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
    puts("Handling popup done");
    assert(shell_surface != NULL);
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done
};

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

    char *name2 = "/run/user/32011/wayland-cursor-shared-cSi17X"; //// TODO
    printf("Creating anonymous file %s...\n", name2);
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
    assert(buffer != NULL);
    if (frame_callback != NULL) {
        wl_callback_destroy(frame_callback);
    }
    if (ht == 0) {
        ht = HEIGHT;
    }

    //  wl_surface@17.frame(new id wl_callback@24)
    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);

    //  wl_surface@17.attach(wl_buffer@25, 0, 0)
    wl_surface_attach(surface, buffer, 0, 0);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);

    //  wl_surface@17.damage(0, 0, 2147483647, 2147483647)
    wl_surface_damage(surface, 0, 0,
                      WIDTH, ht--);
    paint_pixels();

    //  wl_surface@17.commit()
    wl_surface_commit(surface);
}

#ifdef NOTUSED
From File Manager Wayland Log:
[4049499.565] zxdg_surface_v6@20.configure(28)
[4049499.630]  -> zxdg_surface_v6@20.ack_configure(28)
[4049526.045]  -> wl_compositor@5.create_surface(new id wl_surface@23)
[4049526.895]  -> wl_surface@23.destroy()
[4050226.585]  -> wl_surface@17.frame(new id wl_callback@24)
[4050226.792]  -> wl_drm@18.create_prime_buffer(new id wl_buffer@25, fd 25, 720, 1398, 875713112, 0, 2880, 0, 0, 0, 0)
[4050226.976]  -> wl_surface@17.attach(wl_buffer@25, 0, 0)
[4050227.051]  -> wl_surface@17.damage(0, 0, 2147483647, 2147483647)
[4050227.570]  -> wl_surface@17.commit()
#endif  //  NOTUSED

static const struct wl_callback_listener frame_listener = {
    redraw
};

/// Create a Shared Memory Pool
static int create_pool() {
    puts("Creating pool...");
    int stride = WIDTH * 4; // 4 bytes per pixel
    int size = stride * HEIGHT;
    int fd;

    ht = HEIGHT;

    // Open an anonymous file and write some zero bytes to it
    fd = syscall(SYS_memfd_create, "buffer", 0);
    assert(fd >= 0);
    ftruncate(fd, size);
    fallocate(fd, 0, 0, size);

    //  Map it to the memory
    shm_data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(shm_data != MAP_FAILED);

#ifdef NOTUSED
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
#endif  //  NOTUSED

    assert(shm != NULL);
    struct wl_shm_pool *pl = wl_shm_create_pool(shm, fd, size);
    assert(pl != NULL);  wl_display_roundtrip(display);  //  Check for errors

    //  TODO
    //  [4046596.866]  -> wl_shm_pool@9.resize(12288)
    return fd;
}

/// Created a Shared Memory Buffer
static struct wl_buffer *create_buffer(int fd) {
    puts("Creating buffer...");
    int stride = WIDTH * 4; // 4 bytes per pixel
    int size = stride * HEIGHT;
    struct wl_buffer *buff;

    assert(drm != NULL);
    assert(fd >= 0);
    buff = wl_drm_create_prime_buffer(
        drm,
        fd,      //  <arg name="name" type="fd"/>
        WIDTH,   //  <arg name="width" type="int"/>
        HEIGHT,  //  <arg name="height" type="int"/>
        875713112, //  <arg name="format" type="uint"/>
        0,       //  <arg name="offset0" type="int"/>
        stride,  //  <arg name="stride0" type="int"/>
        0,       //  <arg name="offset1" type="int"/>
        0,       //  <arg name="stride1" type="int"/>
        0,       //  <arg name="offset2" type="int"/>
        0        //  <arg name="stride2" type="int"/>
    );
    assert(buff != NULL);  //  wl_display_roundtrip(display);  //  Check for errors

#ifdef NOTUSED
    buff = wl_shm_pool_create_buffer(pool, 0,
                                     WIDTH, HEIGHT,
                                     stride,
                                     WL_SHM_FORMAT_XRGB8888);
    wl_buffer_add_listener(buffer, &buffer_listener, buffer);
    wl_shm_pool_destroy(pool);
#endif  //  NOTUSED

    return buff;
}

#ifdef NOTUSED  //  From File Manager Wayland Log:
[4050226.792]  -> wl_drm@18.create_prime_buffer(
    new id wl_buffer@25, 
    fd 25, 
    720, 
    1398, 
    875713112, 
    0, 
    2880, 
    0, 0, 0, 0)
#endif  //  NOTUSED

/// Create Window
static void create_shm_window(int fd) {
    puts("Creating shared memory window...");
    //  Create a buffer
    buffer = create_buffer(fd);
    assert(buffer != NULL);
    assert(surface != NULL);

    //  wl_surface@17.attach(wl_buffer@25, 0, 0)
    wl_surface_attach(surface, buffer, 0, 0);
    //  wl_display_roundtrip(display);  //  Check for errors

    //  wl_surface@17.damage(0, 0, 2147483647, 2147483647)
    wl_surface_damage(surface, 0, 0, WIDTH, HEIGHT);
    //  wl_display_roundtrip(display);  //  Check for errors

    //  wl_surface@17.commit()
    wl_surface_commit(surface);
    wl_display_roundtrip(display);  //  Check for errors
}

////////////////////////////////////////////////////////////////////
//  Shared Memory

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
    ////  TODO
    if (format == 1) {
        pool_fd = create_pool();
        assert(pool_fd >= 0);
    }
    ////
    fprintf(stderr, "Possible shmem format %s\n", s);
}

struct wl_shm_listener shm_listener = {
    shm_format
};

static void init_shm(void) {
    puts("Init shared memory...");
    pool_fd = create_pool();
    assert(pool_fd >= 0);

    create_shm_window(pool_fd);
}

#ifdef NOTUSED  //  From Wayland Log: logs/shm.log
[2257120.075]  -> wl_surface@3.frame(new id wl_callback@13)
[2257120.384]  -> wl_drm@14.create_prime_buffer(new id wl_buffer@15, fd 10, 480, 360, 875713112, 0, 1920, 0, 0, 0, 0)
[2257120.560]  -> wl_surface@3.attach(wl_buffer@15, 0, 0)
[2257120.613]  -> wl_surface@3.damage(0, 0, 2147483647, 2147483647)
[2257120.708]  -> wl_surface@3.commit()
#endif  //  NOTUSED

////////////////////////////////////////////////////////////////////
//  Direct Rendering Manager

/**
 * device - (none)
 * @name: (none)
 */
void drm_device(void *data,
            struct wl_drm *wl_drm,
            const char *name) {
    printf("DRM device: %s\n", name);

    //  Call this request with the magic received from drmGetMagic().
    //  It will be passed on to the drmAuthMagic() or DRIAuthConnection() call.  
    //  This authentication must be completed before create_buffer could be used.
    //  wl_drm@18.authenticate(4)
    //  TODO: wl_drm_authenticate(wl_drm, ???)
}

#ifdef NOTUSED  //  From https://cgit.freedesktop.org/mesa/mesa/tree/src/egl/drivers/dri2/platform_wayland.c
static void
drm_handle_device(void *data, struct wl_drm *drm, const char *device)
{
   struct dri2_egl_display *dri2_dpy = data;
   drm_magic_t magic;

   dri2_dpy->device_name = strdup(device);
   if (!dri2_dpy->device_name)
      return;

   dri2_dpy->fd = loader_open_device(dri2_dpy->device_name);
   if (dri2_dpy->fd == -1) {
      _eglLog(_EGL_WARNING, "wayland-egl: could not open %s (%s)",
              dri2_dpy->device_name, strerror(errno));
      free(dri2_dpy->device_name);
      dri2_dpy->device_name = NULL;
      return;
   }

   if (drmGetNodeTypeFromFd(dri2_dpy->fd) == DRM_NODE_RENDER) {
      dri2_dpy->authenticated = true;
   } else {
      if (drmGetMagic(dri2_dpy->fd, &magic)) {
         close(dri2_dpy->fd);
         dri2_dpy->fd = -1;
         free(dri2_dpy->device_name);
         dri2_dpy->device_name = NULL;
         _eglLog(_EGL_WARNING, "wayland-egl: drmGetMagic failed");
         return;
      }
      wl_drm_authenticate(dri2_dpy->wl_drm, magic);
   }
}
#endif  //  NOTUSED

/**
 * format - (none)
 * @format: (none)
 */
void drm_format(void *data,
            struct wl_drm *wl_drm,
            uint32_t format) {                
}

/**
 * authenticated - (none)
 */
void drm_authenticated(void *data,
                struct wl_drm *wl_drm) {
}

/**
 * capabilities - (none)
 * @value: (none)
 */
void drm_capabilities(void *data,
                struct wl_drm *wl_drm,
                uint32_t value) {
}

const struct wl_drm_listener drm_listener = {
    .device = drm_device,
    .format = drm_format,
    .authenticated = drm_authenticated,
    .capabilities = drm_capabilities
};

#ifdef NOTUSED  //  From File Manager Wayland Log:
[4046796.038] wl_drm@18.device("/dev/dri/card1")
[4046796.445]  -> wl_drm@18.authenticate(4)
[4046796.511] wl_drm@18.format(875713089)
[4046796.556] wl_drm@18.format(875713112)
[4046796.595] wl_drm@18.format(909199186)
[4046796.633] wl_drm@18.format(961959257)
[4046796.668] wl_drm@18.format(825316697)
[4046796.702] wl_drm@18.format(842093913)
[4046796.740] wl_drm@18.format(909202777)
[4046796.764] wl_drm@18.format(875713881)
[4046796.801] wl_drm@18.format(842094158)
[4046796.841] wl_drm@18.format(909203022)
[4046796.878] wl_drm@18.format(1448695129)
[4046796.917] wl_drm@18.capabilities(1)
[4046796.956] wl_callback@17.done(24)
[4046797.003]  -> wl_display@1.sync(new id wl_callback@17)
[4046797.576] wl_display@1.delete_id(17)
[4046797.660] wl_drm@18.authenticated()
#endif  //  NOTUSED

////////////////////////////////////////////////////////////////////
//  Global Registry

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                        const char *interface, uint32_t version)
{
    //  Note: Version numbers must be correct (last para of wl_registry_bind)
    if (strcmp(interface, "wl_compositor") == 0)
    {
        //  wl_registry@2.bind(3, "wl_compositor", 3, new id [unknown]@5)
        compositor = wl_registry_bind(registry, id,
                                      &wl_compositor_interface,
                                      3);
    }
    else if (strcmp(interface, "zxdg_shell_v6") == 0)
    {
        //  wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@19)
        zshell = wl_registry_bind(registry, id,
                                 &zxdg_shell_v6_interface, 
                                 1);
    }
    else if (strcmp(interface, "wl_shell") == 0)
    {
        shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 
                                 1);
    }
    else if (strcmp(interface, "wl_shm") == 0)
    {
        //  wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@12)
        shm = wl_registry_bind(registry, id,
                               &wl_shm_interface, 
                               1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    }
    else if (strcmp(interface, "wl_drm") == 0)
    {
        //  wl_registry@16.bind(1, "wl_drm", 2, new id [unknown]@18)
        drm = wl_registry_bind(registry, id,
                               &wl_drm_interface, 
                               2);
        //  wl_drm_add_listener(drm, &drm_listener, NULL);
    }
}

static void
global_registry_remover(void *data, struct wl_registry *registry, uint32_t id)
{
    printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
    global_registry_handler,
    global_registry_remover
};

////////////////////////////////////////////////////////////////////
//  XDG Top Level

void xdg_toplevel_configure_handler
(
    void *data,
    struct zxdg_toplevel_v6 *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states
) {
    printf("Configure toplevel: width=%d, height=%d...\n", width, height);
}

void xdg_toplevel_close_handler
(
    void *data,
    struct zxdg_toplevel_v6 *xdg_toplevel
) {
    puts("Closing toplevel...");
}

const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler
};

////////////////////////////////////////////////////////////////////
//  XDG Surface

void xdg_surface_configure_handler
(
    void *data,
    struct zxdg_surface_v6 *xdg_surface,
    uint32_t serial
) {
    puts("Configure XDG surface...");
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
    //  wl_display_roundtrip(display);  //  Check for errors

    if (frame_callback != NULL) {
        wl_callback_destroy(frame_callback);
    }

    //  wl_surface@17.frame(new id wl_callback@24)
    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);

    //  Create the Prime Buffer only when XDG Surface has been configured
    create_shm_window(pool_fd);
    //  wl_display_roundtrip(display);  //  Check for errors

    redraw(NULL, NULL, 0);
    //  wl_display_roundtrip(display);  //  Check for errors
}

const struct zxdg_surface_v6_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler
};

////////////////////////////////////////////////////////////////////
//  XDG Main

static void init_xdg(void) {
    //  zxdg_shell_v6@19.get_xdg_surface(new id zxdg_surface_v6@20, wl_surface@17)
    assert(zshell != NULL);
    zsurface = zxdg_shell_v6_get_xdg_surface(zshell, surface);
    assert(zsurface != NULL);  wl_display_roundtrip(display);  //  Check for errors

    zxdg_surface_v6_add_listener(zsurface, &xdg_surface_listener, NULL);
    wl_display_roundtrip(display);  //  Check for errors

    //  zxdg_surface_v6@20.get_toplevel(new id zxdg_toplevel_v6@21)
    toplevel = zxdg_surface_v6_get_toplevel(zsurface);
    assert(toplevel != NULL);  wl_display_roundtrip(display);  //  Check for errors

    zxdg_toplevel_v6_add_listener(toplevel, &xdg_toplevel_listener, NULL);
    wl_display_roundtrip(display);  //  Check for errors

    //  zxdg_toplevel_v6@21.set_title("com.ubuntu.filemanager")
    zxdg_toplevel_v6_set_title(toplevel, "com.ubuntu.filemanager");
    wl_display_roundtrip(display);  //  Check for errors

    //  zxdg_toplevel_v6@21.set_app_id("filemanager.ubuntu.com.filemanager")
    zxdg_toplevel_v6_set_app_id(toplevel, "filemanager.ubuntu.com.filemanager");
    wl_display_roundtrip(display);  //  Check for errors

    //  wl_surface@17.set_buffer_scale(1)
    wl_surface_set_buffer_scale(surface, 1);
    wl_display_roundtrip(display);  //  Check for errors

    //  wl_surface@17.set_buffer_transform(0)
    wl_surface_set_buffer_transform(surface, 0);
    wl_display_roundtrip(display);  //  Check for errors

    //  Signal that the surface is ready to be configured
    //  wl_surface@17.commit()
    wl_surface_commit(surface);
    wl_display_roundtrip(display);  //  Check for errors
}

////////////////////////////////////////////////////////////////////
//  Display

/**
 * error - fatal error event
 * @object_id: (none)
 * @code: (none)
 * @message: (none)
 *
 * The error event is sent out when a fatal (non-recoverable)
 * error has occurred. The object_id argument is the object where
 * the error occurred, most often in response to a request to that
 * object. The code identifies the error and is defined by the
 * object interface. As such, each interface defines its own set of
 * error codes. The message is an brief description of the error,
 * for (debugging) convenience.
 */
void display_error(void *data,
            struct wl_display *wl_display,
            void *object_id,
            uint32_t code,
            const char *message) {
    printf("*** ERROR %d: %s\n", code, message);
}

/**
 * delete_id - acknowledge object ID deletion
 * @id: (none)
 *
 * This event is used internally by the object ID management
 * logic. When a client deletes an object, the server will send
 * this event to acknowledge that it has seen the delete request.
 * When the client receive this event, it will know that it can
 * safely reuse the object ID.
 */
void display_delete_id(void *data,
            struct wl_display *wl_display,
            uint32_t id) {
    printf("Deleting display %d...\n", id);
}
            
const struct wl_display_listener display_listener = {
    .error = display_error,
    .delete_id = display_delete_id
};

////////////////////////////////////////////////////////////////////
//  EGL

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

/// Render the GLES2 display
static void render_display() {
    puts("Rendering display...");
    glClearColor(1.0f, 0.0f, 1.0f, 1.0f); // Set background color to magenta and opaque
    glClear(GL_COLOR_BUFFER_BIT);         // Clear the color buffer (background)

    glFlush(); // Render now
}

static void create_opaque_region() {
    puts("Creating opaque region...");
    region = wl_compositor_create_region(compositor);
    assert(region != NULL);

    wl_region_add(region, 0, 0,
                  480,
                  360);
    wl_surface_set_opaque_region(surface, region);
}

static void create_egl_window() {
    puts("Creating window...");
    assert(surface != NULL);
    egl_window = wl_egl_window_create(surface,
                                      480, 360);
    if (egl_window == EGL_NO_SURFACE) {
        fprintf(stderr, "Can't create egl window\n");
        exit(1);
    } else {
        fprintf(stderr, "Created egl window\n");
    }

    egl_surface = eglCreateWindowSurface(egl_display, egl_conf,
                               egl_window, NULL);
    assert(egl_surface != NULL);

    if (eglMakeCurrent(egl_display, egl_surface,
                       egl_surface, egl_context)) {
        fprintf(stderr, "Made current\n");
    } else {
        fprintf(stderr, "Made current failed\n");
    }

    // Render the display
    render_display();

    if (eglSwapBuffers(egl_display, egl_surface)) {
        fprintf(stderr, "Swapped buffers\n");
    } else {
        fprintf(stderr, "Swapped buffers failed\n");
    }
}

////////////////////////////////////////////////////////////////////
//  Shell

static void init_shell(void) {
    puts("Init shell...");

    //  Create the shell surface
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    assert(shell_surface != NULL);

    //  Set the shell surface as top level
    wl_shell_surface_set_toplevel(shell_surface);
    wl_display_roundtrip(display);  //  Check for errors

    //  Create the EGL region
    create_opaque_region();
    wl_display_roundtrip(display);  //  Check for errors

    //  Authenticate the display before creating buffers
    init_egl();
    wl_display_roundtrip(display);  //  Check for errors

    //  Create the EGL window
    create_egl_window();
    wl_display_roundtrip(display);  //  Check for errors
}

////////////////////////////////////////////////////////////////////
//  Main

int main(int argc, char **argv) {
    // Show command-line parameters
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]=%s\n", i, argv[i]);
    }
    // Show environment
    for (int i = 0; environ[i] != NULL; i++) {
        puts(environ[i]);
    }
    
    puts("Connecting to display...");
    display = wl_display_connect(NULL);
    assert(display != NULL);

    struct wl_registry *registry = wl_display_get_registry(display);
    assert(registry != NULL);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    //  Wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);
    assert(compositor != NULL);
    assert(shell != NULL);
    assert(drm != NULL);

    //  Create the surface
    surface = wl_compositor_create_surface(compositor);
    assert(surface != NULL);  wl_display_roundtrip(display);  //  Check for errors

    //  Init the shell
    init_shell();

    //  Init shared memory
    init_shm();

#ifdef USE_XDG
    ////  TODO
    init_xdg();
#endif  //  USE_XDG

    puts("Dispatching display...");
    while (wl_display_dispatch(display) != -1)
    {
        ;
    }

    puts("Disconnecting display...");
    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}

#ifdef NOTUSED
From File Manager Wayland Log:
[4049436.500]  -> wl_compositor@5.create_surface(new id wl_surface@17)
[4049437.181]  -> wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@19)
Using the 'xdg-shell-v6' shell integration
[4049437.402]  -> zxdg_shell_v6@19.get_xdg_surface(new id zxdg_surface_v6@20, wl_surface@17)
[4049437.469]  -> zxdg_surface_v6@20.get_toplevel(new id zxdg_toplevel_v6@21)
[4049437.603]  -> zxdg_toplevel_v6@21.set_title("com.ubuntu.filemanager")
[4049437.717]  -> zxdg_toplevel_v6@21.set_app_id("filemanager.ubuntu.com.filemanager")
[4049437.757]  -> wl_surface@17.set_buffer_scale(1)
[4049437.882]  -> wl_surface@17.set_buffer_transform(0)
[4049437.921]  -> wl_surface@17.commit()
#endif  //  NOTUSED
