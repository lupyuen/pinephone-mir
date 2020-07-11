set -e -x
gcc \
    -g \
    -o gtk \
    gtk.c \
    `pkg-config --cflags --libs gtk+-wayland-3.0` \
    -Wl,-Map=gtk.map

sudo mount -o remount,rw /
sudo cp gtk /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app

echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
