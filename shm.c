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
    assert(frame_callback != NULL);

    wl_callback_destroy(frame_callback);
    if (ht == 0)
        ht = HEIGHT;

    frame_callback = wl_surface_frame(surface);
    assert(frame_callback != NULL);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_callback_add_listener(frame_callback, &frame_listener, NULL);
    wl_surface_damage(surface, 0, 0,
                      WIDTH, ht--);
    paint_pixels();
    wl_surface_commit(surface);
}

/*
[4049499.565] zxdg_surface_v6@20.configure(28)
[4049499.630]  -> zxdg_surface_v6@20.ack_configure(28)
[4049526.045]  -> wl_compositor@5.create_surface(new id wl_surface@23)
[4049526.895]  -> wl_surface@23.destroy()
[4050226.585]  -> wl_surface@17.frame(new id wl_callback@24)
[4050226.792]  -> wl_drm@18.create_prime_buffer(new id wl_buffer@25, fd 25, 720, 1398, 875713112, 0, 2880, 0, 0, 0, 0)
[4050226.976]  -> wl_surface@17.attach(wl_buffer@25, 0, 0)
[4050227.051]  -> wl_surface@17.damage(0, 0, 2147483647, 2147483647)
[4050227.570]  -> wl_surface@17.commit()
*/

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
    assert(drm != NULL);
    buff = wl_create_prime_buffer(
        drm,
        fd,      //  <arg name="name" type="fd"/>
        WIDTH,   //  <arg name="width" type="int"/>
        HEIGHT,  //  <arg name="height" type="int"/>
        WL_SHM_FORMAT_XRGB8888, //  <arg name="format" type="uint"/>
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

/*
[4050226.792]  -> wl_drm@18.create_prime_buffer(
    new id wl_buffer@25, 
    fd 25, 
    720, 
    1398, 
    875713112, 
    0, 
    2880, 
    0, 0, 0, 0)
*/

static void
create_window()
{
    puts("Creating window...");
    buffer = create_buffer();
    assert(buffer != NULL);
    assert(surface != NULL);

    //// TODO
    wl_surface_set_buffer_scale(surface, 1);
    wl_surface_set_buffer_transform(surface, 0);
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
Output:

++ gcc -g -o shm shm.c -lwayland-client -Wl,-Map=shm.map
++ sudo mount -o remount,rw /
++ sudo cp shm /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
-rwxr-xr-x 1 clickpkg clickpkg 33928 Jul 15 16:43 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
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
remote: Enumerating objects: 5, done.
remote: Counting objects: 100% (5/5), done.
remote: Compressing objects: 100% (1/1), done.
remote: Total 3 (delta 2), reused 3 (delta 2), pack-reused 0
Unpacking objects: 100% (3/3), done.
From https://github.com/lupyuen/pinephone-mir
   427e005..7ec6e3d  master     -> origin/master
Updating 427e005..7ec6e3d
Fast-forward
 shm.c | 1 -
 1 file changed, 1 deletion(-)
++ gcc -g -o shm shm.c -lwayland-client -Wl,-Map=shm.map
++ sudo mount -o remount,rw /
++ sudo cp shm /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
++ sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
-rwxr-xr-x 1 clickpkg clickpkg 33912 Jul 15 16:43 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
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
[3967421.999]  -> wl_display@1.get_registry(new id wl_registry@2)
[3967442.634] wl_registry@2.global(1, "wl_drm", 2)
[3967442.788] wl_registry@2.global(2, "qt_windowmanager", 1)
[3967442.885] wl_registry@2.global(3, "wl_compositor", 4)
[3967442.979]  -> wl_registry@2.bind(3, "wl_compositor", 1, new id [unknown]@3)
[3967443.085] wl_registry@2.global(4, "wl_subcompositor", 1)
[3967443.165] wl_registry@2.global(5, "wl_seat", 6)
[3967443.244] wl_registry@2.global(6, "wl_output", 3)
[3967443.326] wl_registry@2.global(7, "wl_data_device_manager", 3)
[3967443.405] wl_registry@2.global(8, "wl_shell", 1)
[3967443.489]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@4)
[3967443.595] wl_registry@2.global(9, "zxdg_shell_v6", 1)
[3967443.680] wl_registry@2.global(10, "xdg_wm_base", 1)
[3967443.758] wl_registry@2.global(11, "wl_shm", 1)
[3967443.836]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@5)
[3967443.956]  -> wl_display@1.sync(new id wl_callback@6)
[3967444.250] wl_display@1.delete_id(6)
[3967444.299] wl_shm@5.format(0)
Possible shmem format ARGB8888
[3967444.365] wl_shm@5.format(1)
Possible shmem format XRGB8888
[3967444.419] wl_callback@6.done(0)
Found compositor
[3967444.538]  -> wl_compositor@3.create_surface(new id wl_surface@6)
Created surface
[3967444.592]  -> wl_shell@4.get_shell_surface(new id wl_shell_surface@7, wl_surface@6)
Created shell surface
[3967444.653]  -> wl_shell_surface@7.set_toplevel()
[3967444.694]  -> wl_surface@6.frame(new id wl_callback@8)
Creating window...
Creating buffer...
Creating anonymous file /run/user/32011/weston-shared-XXXXXX...
[3967445.127]  -> wl_shm@5.create_pool(new id wl_shm_pool@9, fd 5, 1024)
[3967445.203]  -> wl_shm_pool@9.create_buffer(new id wl_buffer@10, 0, 16, 16, 64, 1)
[3967445.263]  -> wl_shm_pool@9.destroy()
[3967445.290]  -> wl_surface@6.attach(wl_buffer@10, 0, 0)
[3967445.343]  -> wl_surface@6.commit()
Redrawing...
[3967445.380]  -> wl_surface@6.damage(0, 0, 16, 16)
Painting...
[3967445.556]  -> wl_surface@6.frame(new id wl_callback@11)
[3967445.599]  -> wl_surface@6.attach(wl_buffer@10, 0, 0)
[3967445.636]  -> wl_surface@6.commit()
[3967452.277] wl_display@1.delete_id(9)
[3967452.496] wl_shell_surface@7.configure(0, 720, 1398)
Handling configure: edges=0, width=720, height=1398
disconnected from display
[Inferior 1 (process 23114) exited normally]
No stack.
No stack.
(gdb) quit
Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files
#endif  //  NOTUSED
