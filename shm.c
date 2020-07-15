//  Wayland App that uses Shared Memory to render graphics on PinePhone with Ubuntu Touch.
//  Note: PinePhone display server crashes after the last line "wl_display_disconnect()"
//  To build and run on PinePhone, see shm.sh.
//  Based on https://jan.newmarch.name/Wayland/SharedMemory/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
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

void *shm_data;

int WIDTH = 16;
int HEIGHT = 16;

static void
handle_ping(void *data, struct wl_shell_surface *shell_surface,
            uint32_t serial)
{
    wl_shell_surface_pong(shell_surface, serial);
    fprintf(stderr, "Pinged and ponged\n");
}

static void
handle_configure(void *data, struct wl_shell_surface *shell_surface,
                 uint32_t edges, int32_t width, int32_t height)
{
    printf("Handing configure: edges=%d, width=%d, height=%d...\n", edges, width, height);
}

static void
handle_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
    puts("Handing popup done...");
}

static const struct wl_shell_surface_listener shell_surface_listener = {
    handle_ping,
    handle_configure,
    handle_popup_done
};

static void
buffer_release(void *data, struct wl_buffer *buffer)
{
    puts("Releasing buffer...");
}

static const struct wl_buffer_listener buffer_listener = {
    buffer_release
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
    printf("mkostemp %s\n", tmpname);
    fd = mkostemp(tmpname, O_CLOEXEC);
    if (fd >= 0) {
        unlink(tmpname);
    }
#else
    printf("mkstemp %s\n", tmpname);
    fd = mkstemp(tmpname);
    assert(fd >= 0);
    if (fd >= 0)
    {
        fd = set_cloexec_or_close(fd);
        assert(fd >= 0);
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

    printf("Creating temp file %s...\n", name);
    fd = create_tmpfile_cloexec(name);
    assert(fd >= 0);

    free(name);

    if (fd < 0)
        return -1;

    if (ftruncate(fd, size) < 0)
    {
        assert(0);
        close(fd);
        return -1;
    }

    return fd;
}

static void
paint_pixels()
{
    int n;
    uint32_t *pixel = shm_data;

    fprintf(stderr, "Painting pixels\n");
    for (n = 0; n < WIDTH * HEIGHT; n++)
    {
        *pixel++ = 0xffff;
    }
}

static struct wl_buffer *
create_buffer()
{
    puts("Creating buffer...");
    struct wl_shm_pool *pool;
    int stride = WIDTH * 4; // 4 bytes per pixel
    int size = stride * HEIGHT;
    int fd;
    struct wl_buffer *buff;

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
    assert(pool != NULL);

    ////
    wl_shm_pool_resize(pool, size);
    ////

    buff = wl_shm_pool_create_buffer(pool, 0,
                                     WIDTH, HEIGHT,
                                     stride,
                                     WL_SHM_FORMAT_XRGB8888);
                                     // WL_SHM_FORMAT_ARGB8888);
    assert(buff != NULL);                                     
                            
    wl_buffer_add_listener(buff, &buffer_listener, buff);
    wl_shm_pool_destroy(pool);
    return buff;
}

static void
create_window()
{
    puts("Creating window...");
    buffer = create_buffer();
    assert(buffer != NULL);

    wl_surface_attach(surface, buffer, 0, 0);
    //wl_surface_damage(surface, 0, 0, WIDTH, HEIGHT);
    wl_surface_commit(surface);
}

static void
shm_format(void *data, struct wl_shm *wl_shm, uint32_t format)
{
    //struct display *d = data;

    //	d->formats |= (1 << format);
    fprintf(stderr, "Format %d\n", format);
}

struct wl_shm_listener shm_listener = {
    shm_format};

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
    global_registry_remover};

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
    assert(registry != NULL);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    assert(display != NULL);
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

    create_window();
    paint_pixels();

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
/* Output:
++ sudo mount -o remount,rw /
++ sudo cp shm /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
-rwxr-xr-x 1 clickpkg clickpkg 31584 Jul 15 09:59 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
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
[1202218.042]  -> wl_display@1.get_registry(new id wl_registry@2)
[1202236.790] wl_registry@2.global(1, "wl_drm", 2)
[1202236.943] wl_registry@2.global(2, "qt_windowmanager", 1)
[1202237.016] wl_registry@2.global(3, "wl_compositor", 4)
[1202237.099]  -> wl_registry@2.bind(3, "wl_compositor", 1, new id [unknown]@3)
[1202237.202] wl_registry@2.global(4, "wl_subcompositor", 1)
[1202237.270] wl_registry@2.global(5, "wl_seat", 6)
[1202237.358] wl_registry@2.global(6, "wl_output", 3)
[1202237.420] wl_registry@2.global(7, "wl_data_device_manager", 3)
[1202241.774] wl_registry@2.global(8, "wl_shell", 1)
[1202241.888]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@4)
[1202241.944] wl_registry@2.global(9, "zxdg_shell_v6", 1)
[1202242.027] wl_registry@2.global(10, "xdg_wm_base", 1)
[1202242.100] wl_registry@2.global(11, "wl_shm", 1)
[1202242.183]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@5)
[1202242.320]  -> wl_display@1.sync(new id wl_callback@6)
[1202242.869] wl_display@1.delete_id(6)
[1202242.953] wl_shm@5.format(0)
Format 0
[1202243.081] wl_shm@5.format(1)
Format 1
[1202243.135] wl_callback@6.done(0)
Found compositor
[1202243.200]  -> wl_compositor@3.create_surface(new id wl_surface@6)
Created surface
[1202243.265]  -> wl_shell@4.get_shell_surface(new id wl_shell_surface@7, wl_surface@6)
Created shell surface
[1202243.342]  -> wl_shell_surface@7.set_toplevel()
Creating temp file /run/user/32011/weston-shared-XXXXXX
...mkostemp /run/user/32011/weston-shared-XXXXXX
[1202243.778]  -> wl_shm@5.create_pool(new id wl_shm_pool@8, fd 5, 691200)
[1202243.894]  -> wl_shm_pool@8.create_buffer(new id wl_buffer@9, 0, 480, 360, 1920, 1)
[1202244.012]  -> wl_surface@6.attach(wl_buffer@9, 0, 0)
[1202244.069]  -> wl_surface@6.commit()
Painting pixels
[1202274.917] wl_shell_surface@7.configure(0, 720, 1398)
disconnected from display
[Inferior 1 (process 10732) exited normally]
No stack.
No stack.
(gdb) quit
Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files
*/
