set -e -x
gcc \
    -o wayland \
    wayland.c \
    -lwayland-client \
    -lwayland-server \
    -Wl,-Map=wayland.map

sudo cp wayland /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
