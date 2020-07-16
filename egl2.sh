# Build and install Wayland EGL app

# TODO: Install GLES2 library
# sudo apt install libgles2-mesa-dev

# Kill the app if it's already running
pkill egl2

# Stop the script on error, echo all commands
set -e -x

# Build app
gcc \
    -g \
    -o egl2 \
    egl2.c \
    shader.c \
    texture.c \
    util.c \
    -lwayland-client \
    -lwayland-server \
    -lwayland-egl \
    -L/usr/lib/aarch64-linux-gnu/mesa-egl \
    -lEGL \
    /usr/lib/aarch64-linux-gnu/mesa-egl/libGLESv2.so.2 \
    -Wl,-Map=egl2.map

# Make system folders writeable
sudo mount -o remount,rw /

# Copy app to File Manager folder
sudo cp egl2 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl2
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/egl2

# Copy run script to File Manager folder
# TODO: Check that run.sh contains "./egl2"
sudo cp run.sh /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager

# Start the File Manager
echo "*** Tap on File Manager icon on PinePhone"

# Monitor the log file
echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log

# Press Ctrl-C to stop. To kill the app:
# pkill egl2
