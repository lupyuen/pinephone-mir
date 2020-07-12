# Build and install gtk app

set -e -x

# Build app
gcc \
    -g \
    -o gtk \
    gtk.c \
    `pkg-config --cflags --libs gtk+-wayland-3.0` \
    -Wl,-Map=gtk.map

# Make system folders writeable
sudo mount -o remount,rw /

# Copy app to File Manager folder
sudo cp gtk /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/gtk
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/gtk

# Start the File Manager
echo "*** Tap on File Manager icon on PinePhone"

# Monitor the log file
echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
