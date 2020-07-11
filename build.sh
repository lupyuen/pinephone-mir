set -e -x

git pull

gcc \
    -g \
    -o basic \
    basic.c \
    `pkg-config --libs --cflags mirclient`

sudo mount -o remount,rw /
sudo cp basic /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app

echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log
