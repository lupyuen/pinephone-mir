//  Wayland App that uses Shared Memory to render graphics on PinePhone with Ubuntu Touch.
//  Note: PinePhone display server crashes after the last line "wl_display_disconnect()"
//  Error in /home/phablet/.cache/upstart/unity8.log:
//    terminate called after throwing an instance of 'std::logic_error'
//    what():  Buffer does not support GL rendering
//  To build and run on PinePhone, see shm.sh.
//  Based on https://jan.newmarch.name/Wayland/WhenCanIDraw/
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

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;
struct wl_shm *shm;
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
    assert(data != NULL); assert(callback != NULL);
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
create_window()
{
    puts("Creating window...");
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
    global_registry_remover
};

int main(int argc, char **argv)
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

    if (compositor == NULL)
    {
        fprintf(stderr, "Can't find compositor\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Found compositor\n");
    }

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
    if (shell_surface == NULL)
    {
        fprintf(stderr, "Can't create shell surface\n");
        exit(1);
    }
    else
    {
        fprintf(stderr, "Created shell surface\n");
    }
    wl_shell_surface_set_toplevel(shell_surface);

    wl_shell_surface_add_listener(shell_surface,
                                  &shell_surface_listener, NULL);

    frame_callback = wl_surface_frame(surface);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);

    create_window();
    redraw(NULL, NULL, 0);

    while (wl_display_dispatch(display) != -1)
    {
        ;
    }

    wl_display_disconnect(display);
    printf("disconnected from display\n");

    exit(0);
}

/* Output:
++ gcc -g -o shm shm.c -lwayland-client -Wl,-Map=shm.map
++ sudo mount -o remount,rw /
++ sudo cp shm /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
-rwxr-xr-x 1 clickpkg clickpkg 33152 Jul 15 11:04 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
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
connected to display
[818794.350]  -> wl_display@1.get_registry(new id wl_registry@2)
[818817.316] wl_registry@2.global(1, "wl_drm", 2)
[818817.491] wl_registry@2.global(2, "qt_windowmanager", 1)
[818817.562] wl_registry@2.global(3, "wl_compositor", 4)
[818817.646]  -> wl_registry@2.bind(3, "wl_compositor", 1, new id [unknown]@3)
[818817.763] wl_registry@2.global(4, "wl_subcompositor", 1)
[818817.849] wl_registry@2.global(5, "wl_seat", 6)
[818817.916] wl_registry@2.global(6, "wl_output", 3)
[818817.985] wl_registry@2.global(7, "wl_data_device_manager", 3)
[818818.049] wl_registry@2.global(8, "wl_shell", 1)
[818818.129]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@4)
[818818.252] wl_registry@2.global(9, "zxdg_shell_v6", 1)
[818818.316] wl_registry@2.global(10, "xdg_wm_base", 1)
[818818.376] wl_registry@2.global(11, "wl_shm", 1)
[818818.448]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@5)
[818818.646]  -> wl_display@1.sync(new id wl_callback@6)
[818819.185] wl_display@1.delete_id(6)
[818819.263] wl_shm@5.format(0)
Possible shmem format ARGB8888
[818819.332] wl_shm@5.format(1)
Possible shmem format XRGB8888
[818819.382] wl_callback@6.done(20)
Found compositor
[818819.450]  -> wl_compositor@3.create_surface(new id wl_surface@6)
Created surface
[818819.516]  -> wl_shell@4.get_shell_surface(new id wl_shell_surface@7, wl_surface@6)
Created shell surface
[818819.612]  -> wl_shell_surface@7.set_toplevel()
[818819.648]  -> wl_surface@6.frame(new id wl_callback@8)
[818820.060]  -> wl_shm@5.create_pool(new id wl_shm_pool@9, fd 5, 1024)
[818820.145]  -> wl_shm_pool@9.create_buffer(new id wl_buffer@10, 0, 16, 16, 64, 1)
[818820.302]  -> wl_shm_pool@9.destroy()
[818820.350]  -> wl_surface@6.attach(wl_buffer@10, 0, 0)
[818820.430]  -> wl_surface@6.commit()
[818820.457]  -> wl_surface@6.damage(0, 0, 16, 16)
[818820.633]  -> wl_surface@6.frame(new id wl_callback@11)
[818820.679]  -> wl_surface@6.attach(wl_buffer@10, 0, 0)
[818820.725]  -> wl_surface@6.commit()
[818834.621] wl_display@1.delete_id(9)
[818834.761] wl_shell_surface@7.configure(0, 720, 1398)
disconnected from display
[Inferior 1 (process 12109) exited normally]
No stack.
No stack.
(gdb) quit
Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files
*/
