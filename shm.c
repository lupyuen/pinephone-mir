//  Wayland App that uses Shared Memory to render graphics on PinePhone with Ubuntu Touch.
//  Note: PinePhone display server crashes after the last line "wl_display_disconnect()"
//  Error in /home/phablet/.cache/upstart/unity8.log:
//    terminate called after throwing an instance of 'std::logic_error'
//    what():  Buffer does not support GL rendering
//  To build and run on PinePhone, see shm.sh.
//  Based on inspection of Ubuntu Touch File Manager Wayland Log (logs/filemanager-wayland.log)
//  and https://jan.newmarch.name/Wayland/WhenCanIDraw/
//  and https://bugaevc.gitbooks.io/writing-wayland-clients/beyond-the-black-square/xdg-shell.html
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include <wayland-server-protocol.h>
#include <wayland-egl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#include "xdg-shell.h"
#include "wayland-drm-client-protocol.h"

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct zxdg_shell_v6 *shell;      //  Previously: struct wl_shell *shell;
struct zxdg_surface_v6 *zsurface;  //  Previously: struct wl_shell_surface *shell_surface;
struct zxdg_toplevel_v6 *toplevel;
struct wl_shm *shm;
struct wl_drm *drm;
struct wl_buffer *buffer;
struct wl_callback *frame_callback;

void *shm_data;

int WIDTH = 16;
int HEIGHT = 16;

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

    //  TODO
    //  [4046596.866]  -> wl_shm_pool@9.resize(12288)

    //  TODO
    assert(drm != NULL);
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
    /* TODO
    buff = wl_shm_pool_create_buffer(pool, 0,
                                     WIDTH, HEIGHT,
                                     stride,
                                     WL_SHM_FORMAT_XRGB8888);
    */
    //wl_buffer_add_listener(buffer, &buffer_listener, buffer);
    ////  TODO: wl_shm_pool_destroy(pool);
    return buff;
}

#ifdef NOTUSED
From File Manager Wayland Log:
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

static void
create_window()
{
    puts("Creating window...");
    buffer = create_buffer();
    assert(buffer != NULL);
    assert(surface != NULL);

    //// TODO
    //  wl_surface@17.set_buffer_scale(1)
    wl_surface_set_buffer_scale(surface, 1);

    //  wl_surface@17.set_buffer_transform(0)
    wl_surface_set_buffer_transform(surface, 0);

    //  wl_surface@17.commit()
    wl_surface_commit(surface);
    ////

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, WIDTH, HEIGHT);  //// TODO
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
    shm_format
};

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
                        const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0)
    {
        compositor = wl_registry_bind(registry,
                                      id,
                                      &wl_compositor_interface,
                                      1);
    }
    else if (strcmp(interface, "zxdg_shell_v6") == 0)
    {
        //  wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@19)
        shell = wl_registry_bind(registry, id,
                                 &zxdg_shell_v6_interface, 1);
    }
#ifdef NOTUSED
    else if (strcmp(interface, "wl_shell") == 0)
    {
        shell = wl_registry_bind(registry, id,
                                 &wl_shell_interface, 1);
    }
#endif  //  NOTUSED
    else if (strcmp(interface, "wl_shm") == 0)
    {
        shm = wl_registry_bind(registry, id,
                               &wl_shm_interface, 1);
        wl_shm_add_listener(shm, &shm_listener, NULL);
    }
    else if (strcmp(interface, "wl_drm") == 0)
    {
        drm = wl_registry_bind(registry, id,
                               &wl_drm_interface, 1);
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
//  XDG

void xdg_toplevel_configure_handler
(
    void *data,
    struct zxdg_toplevel_v6 *xdg_toplevel,
    int32_t width,
    int32_t height,
    struct wl_array *states
) {
    printf("configure: %dx%d\n", width, height);
}

void xdg_toplevel_close_handler
(
    void *data,
    struct zxdg_toplevel_v6 *xdg_toplevel
) {
    printf("close\n");
}

const struct zxdg_toplevel_v6_listener xdg_toplevel_listener = {
    .configure = xdg_toplevel_configure_handler,
    .close = xdg_toplevel_close_handler
};

void xdg_surface_configure_handler
(
    void *data,
    struct zxdg_surface_v6 *xdg_surface,
    uint32_t serial
) {
    zxdg_surface_v6_ack_configure(xdg_surface, serial);
}

const struct zxdg_surface_v6_listener xdg_surface_listener = {
    .configure = xdg_surface_configure_handler
};

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
//  Main

int main(int argc, char **argv)
{
    puts("Connecting to display...");
    display = wl_display_connect(NULL);
    assert(display != NULL);

    wl_display_add_listener(display, &display_listener, NULL);

    struct wl_registry *registry = wl_display_get_registry(display);
    assert(registry != NULL);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);
    assert(compositor != NULL);
    assert(shell != NULL);

    surface = wl_compositor_create_surface(compositor);
    assert(surface != NULL);

    ////  TODO
    //  zxdg_shell_v6@19.get_xdg_surface(new id zxdg_surface_v6@20, wl_surface@17)
    zsurface = zxdg_shell_v6_get_xdg_surface(shell, surface);
    assert(zsurface != NULL);

    zxdg_surface_v6_add_listener(zsurface, &xdg_surface_listener, NULL);

    //  zxdg_surface_v6@20.get_toplevel(new id zxdg_toplevel_v6@21)
    toplevel = zxdg_surface_v6_get_toplevel(zsurface);
    assert(toplevel != NULL);

    zxdg_toplevel_v6_add_listener(toplevel, &xdg_toplevel_listener, NULL);

    //  zxdg_toplevel_v6@21.set_title("com.ubuntu.filemanager")
    zxdg_toplevel_v6_set_title(toplevel, "com.ubuntu.filemanager");

    //  zxdg_toplevel_v6@21.set_app_id("filemanager.ubuntu.com.filemanager")
    zxdg_toplevel_v6_set_app_id(toplevel, "filemanager.ubuntu.com.filemanager");

    // signal that the surface is ready to be configured
    // wl_surface_commit(surface);

    // ...
    // create a pool
    // create a buffer
    // ...

    // wait for the surface to be configured
    // wl_display_roundtrip(display);

    // wl_surface_attach(surface, buffer, 0, 0);
    // wl_surface_commit(surface);
    ////

#ifdef NOTUSED
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    assert(shell_surface != NULL);

    wl_shell_surface_set_toplevel(shell_surface);

    wl_shell_surface_add_listener(shell_surface,
                                  &shell_surface_listener, NULL);

    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);

    wl_callback_add_listener(frame_callback, &frame_listener, NULL);
#endif  //  NOTUSED

    create_window();
    redraw(NULL, NULL, 0);

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

#ifdef NOTUSED
Output:

++ gcc -g -o shm shm.c xdg-shell.c wayland-drm-protocol.c -lwayland-client -Wl,-Map=shm.map
++ sudo mount -o remount,rw /
++ sudo cp shm /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
-rwxr-xr-x 1 clickpkg clickpkg 44496 Jul 15 21:48 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
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
Reading symbols from ./shm...done.
Starting program: /usr/share/click/preinstalled/com.ubuntu.filemanager/0.7.5/shm 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
Connecting to display...
[806924.394]  -> wl_display@1.get_registry(new id wl_registry@2)
[806924.599]  -> wl_display@1.sync(new id wl_callback@3)
[806944.909] wl_display@1.delete_id(3)
[806945.303] wl_registry@2.global(1, "wl_drm", 2)
[806945.434]  -> wl_registry@2.bind(1, "wl_drm", 1, new id [unknown]@4)
[806945.520] wl_registry@2.global(2, "qt_windowmanager", 1)
[806945.579] wl_registry@2.global(3, "wl_compositor", 4)
[806945.623]  -> wl_registry@2.bind(3, "wl_compositor", 1, new id [unknown]@5)
[806945.668] wl_registry@2.global(4, "wl_subcompositor", 1)
[806945.703] wl_registry@2.global(5, "wl_seat", 6)
[806945.758] wl_registry@2.global(6, "wl_output", 3)
[806945.815] wl_registry@2.global(7, "wl_data_device_manager", 3)
[806945.867] wl_registry@2.global(8, "wl_shell", 1)
[806945.918] wl_registry@2.global(9, "zxdg_shell_v6", 1)
[806945.981]  -> wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@6)
[806946.054] wl_registry@2.global(10, "xdg_wm_base", 1)
[806946.103] wl_registry@2.global(11, "wl_shm", 1)
[806946.152]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@7)
[806946.226] wl_callback@3.done(86)
[806946.264]  -> wl_compositor@5.create_surface(new id wl_surface@3)
[806946.313]  -> zxdg_shell_v6@6.get_xdg_surface(new id zxdg_surface_v6@8, wl_surface@3)
[806946.367]  -> zxdg_surface_v6@8.get_toplevel(new id zxdg_toplevel_v6@9)
[806950.474]  -> zxdg_toplevel_v6@9.set_title("com.ubuntu.filemanager")
[806950.522]  -> zxdg_toplevel_v6@9.set_app_id("filemanager.ubuntu.com.filemanager")
Creating window...
Creating buffer...
Creating anonymous file /run/user/32011/weston-shared-XXXXXX...
[806950.934]  -> wl_shm@7.create_pool(new id wl_shm_pool@10, fd 5, 1024)
[806951.004]  -> wl_drm@4.create_prime_buffer(new id wl_buffer@11, fd 6, 16, 16, 875713112, 0, 64, 0, 0, 0, 0)
[806951.113]  -> wl_surface@3.set_buffer_scale(1)
[806951.141]  -> wl_surface@3.set_buffer_transform(0)
[806951.169]  -> wl_surface@3.commit()
[806951.195]  -> wl_surface@3.attach(wl_buffer@11, 0, 0)
[806951.239]  -> wl_surface@3.damage(0, 0, 16, 16)
[806951.286]  -> wl_surface@3.commit()
Redrawing...
[806951.316]  -> wl_surface@3.frame(new id wl_callback@12)
[806951.349]  -> wl_surface@3.attach(wl_buffer@11, 0, 0)
[806951.392]  -> wl_surface@3.damage(0, 0, 16, 16)
Painting...
[806951.510]  -> wl_surface@3.commit()
Dispatching display...
[806953.011] wl_display@1.error(wl_display@1, 1, "invalid method 3 (since 1 < 2), object wl_drm@4")
wl_display@1: error 1: invalid method 3 (since 1 < 2), object wl_drm@4
Disconnecting display...
disconnected from display
[Inferior 1 (process 22372) exited normally]
No stack.
No stack.
(gdb) quit
Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files

#endif  //  NOTUSED
