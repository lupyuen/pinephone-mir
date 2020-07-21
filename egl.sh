# Build and install Wayland EGL app

set -e -x

# Build app
gcc \
    -g \
    -o egl \
    egl.c \
    -Wl,-Map=egl.map \
    -L/usr/lib/aarch64-linux-gnu/mesa-egl \
    -lwayland-client \
    -lwayland-server \
    -lwayland-egl \
    -lEGL \
    -lGLESv2

# Make system folders writeable
sudo mount -o remount,rw /

# Copy app to File Manager folder
sudo cp egl /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl

# Start the File Manager
echo "*** Tap on File Manager icon on PinePhone"

# Monitor the log file
echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
