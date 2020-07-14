//  Wayland App that uses Shared Memory to render graphics on PinePhone with Ubuntu Touch.
//  Note: This app causes PinePhone to crash after the last line "wl_display_disconnect()".
//  To build and run on PinePhone, see shm.sh.
//  Based on https://bugaevc.gitbooks.io/writing-wayland-clients/black-square/the-complete-code.html
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <syscall.h>
#include <unistd.h>
#include <sys/mman.h>

#include <wayland-client.h>

struct wl_compositor *compositor;
struct wl_shm *shm;
struct wl_shell *shell;

void registry_global_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version
) {
    if (strcmp(interface, "wl_compositor") == 0) {
        puts("Got compositor");
        compositor = wl_registry_bind(registry, name,
            &wl_compositor_interface, 3);
    } else if (strcmp(interface, "wl_shm") == 0) {
        puts("Got shared memory");
        shm = wl_registry_bind(registry, name,
            &wl_shm_interface, 1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        puts("Got shell");
        shell = wl_registry_bind(registry, name,
            &wl_shell_interface, 1);
    }
}

void registry_global_remove_handler
(
    void *data,
    struct wl_registry *registry,
    uint32_t name
) {}

const struct wl_registry_listener registry_listener = {
    .global = registry_global_handler,
    .global_remove = registry_global_remove_handler
};

int main(void)
{
    struct wl_display *display = wl_display_connect(NULL);
    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    // wait for the "initial" set of globals to appear
    wl_display_roundtrip(display);

    puts("Creating surface...");
    assert(compositor != NULL);    
    struct wl_surface *surface = wl_compositor_create_surface(compositor);
    assert(surface != NULL);

    assert(shell != NULL);
    puts("Getting shell surface...");
    struct wl_shell_surface *shell_surface = wl_shell_get_shell_surface(shell, surface);
    assert(shell_surface != NULL);

    puts("Setting top level shell surface...");
    wl_shell_surface_set_toplevel(shell_surface);

    int width = 200;
    int height = 200;
    int stride = width * 4;
    int size = stride * height;  // bytes

    // open an anonymous file and write some zero bytes to it
    puts("Creating anonymous file...");
    int fd = syscall(SYS_memfd_create, "buffer", 0);
    assert(fd >= 0);

    puts("Truncating anonymous file...");
    ftruncate(fd, size);

    // map it to the memory
    puts("Mapping anonymous file to memory...");
    unsigned char *data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(data != NULL);

    // turn it into a shared memory pool
    assert(shell != NULL);
    puts("Creating shared memory pool...");
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
    assert(pool != NULL);

    // allocate the buffer in that pool
    puts("Creating shared memory pool buffer...");
    struct wl_buffer *buffer = wl_shm_pool_create_buffer(pool,
        0, width, height, stride, WL_SHM_FORMAT_XRGB8888);
    assert(buffer != NULL);

    puts("Attaching surface...");
    wl_surface_attach(surface, buffer, 0, 0);

    puts("Commiting surface...");
    wl_surface_commit(surface);

    puts("Starting display loop...");
    while (wl_display_dispatch(display) != -1)
    {
        ;
    }

    puts("Disconnecting from display...");
    wl_display_disconnect(display);
}