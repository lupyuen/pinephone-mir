/* Connect to Wayland display and create shell surface
(1) Make a copy of the camera app:

cp /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app $HOME

(2) Build the Wayland app:

gcc \
    -o wayland \
    wayland.c \
    -lwayland-client \
    -lwayland-server \
    -Wl,-Map=wayland.map

(3) Overwrite the camera app with the GTK app:

sudo mount -o remount,rw /
sudo cp wayland /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app

(4) Run the camera app

(5) Check Logviewer for the output under camera app:

Copy files to PinePhone:
scp -i ~/.ssh/id_rsa wayland.* phablet@192.168.1.139:

Based on https://jan.newmarch.name/Wayland/ProgrammingClient/
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-server.h>
#include <wayland-client-protocol.h>
#include <wayland-egl.h>

extern char **environ;

struct wl_display *display = NULL;
struct wl_compositor *compositor = NULL;
struct wl_surface *surface;
struct wl_shell *shell;
struct wl_shell_surface *shell_surface;

static void
global_registry_handler(void *data, struct wl_registry *registry, uint32_t id,
	       const char *interface, uint32_t version)
{
    if (strcmp(interface, "wl_compositor") == 0) {
        //  Get Wayland compositor
        compositor = wl_registry_bind(registry, 
				      id, 
				      &wl_compositor_interface, 
				      1);
    } else if (strcmp(interface, "wl_shell") == 0) {
        //  Get Wayland shell
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
    global_registry_remover
};

int main(int argc, char **argv) {
    //  Show command-line parameters
    for (int i = 0; i < argc; i++) {
        printf("argv[%d]=%s\n", i, argv[i]);
    }
    //  Show environment
    for (int i = 0; environ[i] != NULL; i++) {
        puts(environ[i]);
    }

    //  Connect to Wayland display
    display = wl_display_connect(NULL);
    if (display == NULL) {
    	fprintf(stderr, "Can't connect to Wayland display\n");
    	exit(1);
    }
    printf("Connected to Wayland display\n");

    struct wl_registry *registry = wl_display_get_registry(display);
    wl_registry_add_listener(registry, &registry_listener, NULL);

    //  Get Wayland compositor
    wl_display_dispatch(display);
    wl_display_roundtrip(display);

    if (compositor == NULL) {
	    fprintf(stderr, "Can't find Wayland compositor\n");
	    exit(1);
    } else {
	    fprintf(stderr, "Found Wayland compositor\n");
    }

    //  Create Wayland surface
    surface = wl_compositor_create_surface(compositor);
    if (surface == NULL) {
	    fprintf(stderr, "Can't create Wayland surface\n");
	    exit(1);
    } else {
	    fprintf(stderr, "Created Wayland surface\n");
    }

    //  Create Wayland shell surface
    shell_surface = wl_shell_get_shell_surface(shell, surface);
    if (shell_surface == NULL) {
	    fprintf(stderr, "Can't create Wayland shell surface\n");
	    exit(1);
    } else {
	    fprintf(stderr, "Created Wayland shell surface\n");
    }
    wl_shell_surface_set_toplevel(shell_surface);

    //  Disconnect Wayland display
    wl_display_disconnect(display);
    printf("Disconnected from Wayland display\n");
    
    exit(0);
}

/* Output:

*/