set -e -x
gcc \
    -o egl \
    egl.c \
    -lwayland-client \
    -lwayland-server \
    -lwayland-egl \
    -L/usr/lib/aarch64-linux-gnu/mesa-egl \
    -lEGL \
    -lGLESv2 \
    -Wl,-Map=egl.map

sudo mount -o remount,rw /
sudo cp egl /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app

echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
