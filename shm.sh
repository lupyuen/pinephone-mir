# Build and install Wayland Shared Memory app

# Kill the app if it's already running
pkill shm

# Stop the script on error, echo all commands
set -e -x

# Build app
gcc \
    -g \
    -o shm \
    shm.c \
    -lwayland-client \
    -Wl,-Map=shm.map

# Make system folders writeable
sudo mount -o remount,rw /

# Copy app to File Manager folder
sudo cp shm /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/shm

# Copy run script to File Manager folder
# TODO: Check that run.sh contains "./shm"
sudo cp run.sh /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager

# Start the File Manager
echo "*** Tap on File Manager icon on PinePhone"

# Monitor the log file
echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log

# Press Ctrl-C to stop. To kill the app:
# pkill shm
