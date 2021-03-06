set -e -x

gcc \
    -o wayland \
    wayland.c \
    -lwayland-client \
    -lwayland-server \
    -Wl,-Map=wayland.map

sudo mount -o remount,rw /
sudo cp wayland /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app

echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
