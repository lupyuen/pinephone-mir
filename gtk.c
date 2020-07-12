/* Simple GTK3 App for Ubuntu Touch on PinePhone (Not working yet)
(1) Make a copy of the camera app:

cp /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app $HOME

(2) Build the GTK app:

gcc \
    -g \
    -o gtk \
    gtk.c \
    `pkg-config --cflags --libs gtk+-wayland-3.0` \
    -Wl,-Map=gtk.map

(3) Overwrite the camera app with the GTK app:

sudo mount -o remount,rw /
sudo cp gtk /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app

(4) Run the camera app

Tap the Camera icon in the menu

Check for errors in /var/log/syslog, /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log

Also check in Logviewer under "v3.1.3 camera", "dbus" and "unity8"

Logviewer hangs if the log file is too huge. If it happens, delete the log: /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log

If gtk_application_new() name is NULL:

  See complete log at https://gist.github.com/lupyuen/2380a6a6f121d16ce7588f77497c7e1d

  (camera-app:10232): dconf-WARNING **: unable to open named profile (/tmp/camera-dconf): using the null configuration.

  ** (camera-app:10232): WARNING **: Error retrieving accessibility bus address: org.freedesktop.DBus.Error.ServiceUnknown: The name org.a11y.Bus was not provided by any .service files
  (camera-app:10232): Gtk-WARNING **: Failed to parse /etc/gtk-3.0/settings.ini: Permission denied

  Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files

  Jul 10 15:41:34 ubuntu-phablet kernel: [ 3442.203468] audit: type=1400 audit(1594366894.725:196): 
  apparmor="DENIED" operation="open" profile="com.ubuntu.camera_camera_3.1.3" name="/etc/gtk-3.0/settings.ini" pid=10232 comm="camera-app" requested_mask="r" denied_mask="r" fsuid=32011 ouid=0

If gtk_application_new() name is "com.ubuntu.camera":

  Failed to register: GDBus.Error:org.freedesktop.DBus.Error.AccessDenied: 
  Connection ":1.101" is not allowed to own the service "com.ubuntu.camera" 
  due to AppArmor policy

  Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name 
  com.canonical.PropertyService was not provided by any .service files

  dbus[2936]: apparmor="DENIED" operation="dbus_bind"  bus="session" 
  name="com.ubuntu.camera" mask="bind" pid=8310 label="com.ubuntu.camera_camera_3.1.3"

If gtk_application_new() name is "com.ubuntu.camera_camera_3.1.3":

  (process:13280): Gtk-CRITICAL **: gtk_application_new: assertion 
  'application_id == NULL || g_application_id_is_valid (application_id)' failed

  (process:13280): GLib-GObject-WARNING **: invalid (NULL) pointer instance

  (process:13280): GLib-GObject-CRITICAL **: g_signal_connect_data: 
  assertion 'G_TYPE_CHECK_INSTANCE (instance)' failed

  (process:13280): GLib-GIO-CRITICAL **: g_application_run: assertion 
  'G_IS_APPLICATION (application)' failed

  Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: 
  The name com.canonical.PropertyService was not provided by any .service files

Tip: To copy files to PinePhone via SSH:
scp -i ~/.ssh/id_rsa gtk.* phablet@192.168.1.160:

View AppArmor Profiles:
sudo apt install apparmor-utils
sudo aa-confined
cat /etc/apparmor.d/usr.bin.messaging-app

Based on the C sample from https://www.gtk.org
*/
#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>

extern char **environ;

/// Handle activation of our app by rendering the UI controls
static void on_activate (GtkApplication *app) {
  puts("App activated");

  // Create a new window
  puts("Create window...");
  GtkWidget *window = gtk_application_window_new (app);

  // Create a new button
  // puts("Create button...");
  // GtkWidget *button = gtk_button_new_with_label ("Hello, World!");

  // When the button is clicked, destroy the window passed as an argument
  // puts("Connect button...");
  // g_signal_connect_swapped (button, "clicked", G_CALLBACK (gtk_widget_destroy), window);

  // Add the button to the window
  // puts("Add button...");
  // gtk_container_add (GTK_CONTAINER (window), button);

  // Show the window
  puts("Show window...");
  gtk_widget_show_all (window);
}

int main (int argc, char *argv[]) {
  // Set DCONF profile, because the default profile folder is blocked by AppArmor: run/user/32011/dconf/
  setenv("DCONF_PROFILE", "/tmp/camera-dconf", 1);

  // Set GTK backend to Wayland
  // Default GTK socket: /run/user/32011/wayland-0
  // setenv("GDK_BACKEND", "wayland", 1);

  // Set GTK backend to Mir
  // Default Mir socket: /run/user/32011/mir_socket
  // setenv("GDK_BACKEND", "mir", 1);
  // setenv("MIR_SOCKET", "/run/user/32011/mir_socket", 1);

  // Show GTK debugging messages
  setenv("GTK_DEBUG", "actions,builder,geometry,icontheme,keybindings,misc,modules,plugsocket,pixel-cache,printing,size-request,text,tree", 1);

  // Show command-line parameters
  for (int i = 0; i < argc; i++) {
    printf("argv[%d]=%s\n", i, argv[i]);
  }
  // Show environment
  for (int i = 0; environ[i] != NULL; i++) {
    puts(environ[i]);
  }
  // Create a new application
  // Note: Name must be NULL or the app will be blocked by AppArmor
  puts("Create app...");
  GtkApplication *app = gtk_application_new (
    NULL,  // Name must be NULL to prevent AppArmor dbus_bind error
    G_APPLICATION_NON_UNIQUE  // Don't check for other instances of this app. Previously G_APPLICATION_FLAGS_NONE
  );

  // Set the app activation callback
  puts("Connect app...");
  g_signal_connect (app, "activate", G_CALLBACK (on_activate), NULL);

  // Run the app. Will block until the app terminates.
  puts("Run app...");
  int status = g_application_run (G_APPLICATION (app), argc, argv);

  printf("Exit status %d\n", status);
}

/* Output:
+++ pkg-config --cflags --libs gtk+-wayland-3.0
++ gcc -g -o gtk gtk.c -pthread -I/usr/include/gtk-3.0 -I/usr/include/at-spi2-atk/2.0 -I/usr/include/at-spi-2.0 -I/usr/include/dbus-1.0 -I/usr/lib/aarch64-linux-gnu/dbus-1.0/include -I/usr/include/gtk-3.0 -I/usr/include/gio-unix-2.0/ -I/usr/include/mirclient -I/usr/include/mircore -I/usr/include/mircookie -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/freetype2 -I/usr/include/libpng12 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng12 -I/usr/include/glib-2.0 -I/usr/lib/aarch64-linux-gnu/glib-2.0/include -lgtk-3 -lgdk-3 -lpangocairo-1.0 -lpango-1.0 -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0 -Wl,-Map=gtk.map
++ sudo mount -o remount,rw /
++ sudo cp gtk /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
++ ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
-rwxr-xr-x 1 clickpkg clickpkg 17656 Jul 10 19:54 /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app
++ echo
++ tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.camera_camera_3.1.3.log

argv[0]=./camera-app
PATH=/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/lib/aarch64-linux-gnu/bin:/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera:/home/phablet/go-1.14.4/bin:/home/phablet/go-1.13.6/bin:/usr/lib/go-1.13.6/bin:/home/phablet/bin:/home/phablet/.local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
TERM=linux
GRID_UNIT_PX=14
LANGUAGE=en_US:en
USER=phablet
XDG_SEAT=seat0
EXTERNAL_STORAGE=/mnt/sdcard
HOSTNAME=android
XDG_SESSION_TYPE=mir
LOOP_MOUNTPOINT=/mnt/obb
SSH_AGENT_PID=2892
SHLVL=1
HOME=/home/phablet
DESKTOP_SESSION=ubuntu-touch
XDG_SEAT_PATH=/org/freedesktop/DisplayManager/Seat0
ANDROID_ASSETS=/system/app
GOROOT=
QML2_IMPORT_PATH=/usr/lib/aarch64-linux-gnu/qt5/imports:/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/lib/aarch64-linux-gnu
DBUS_SESSION_BUS_ADDRESS=unix:abstract=/tmp/dbus-MFuzagleui
QT_EXCLUDE_GENERIC_BEARER=1
QT_WAYLAND_DISABLE_WINDOWDECORATION=1
QT_QPA_PLATFORMTHEME=ubuntuappmenu
GTK_IM_MODULE=Maliit
LOGNAME=phablet
QT_SELECT=qt5
QT_WAYLAND_FORCE_DPI=96
QTWEBKIT_DPR=1
FLASH_KERNEL_SKIP=true
XDG_SESSION_ID=c1
QT_FILE_SELECTORS=ubuntu-touch
MKSH=/system/bin/sh
ANDROID_DATA=/data
GDM_LANG=en_US
XDG_SESSION_PATH=/org/freedesktop/DisplayManager/Session0
ANDROID_ROOT=/system
XDG_RUNTIME_DIR=/run/user/32011
LANG=en_US.UTF-8
XDG_CURRENT_DESKTOP=Unity
MIR_SERVER_WAYLAND_HOST=/tmp/wayland-root
XDG_SESSION_DESKTOP=ubuntu-touch
QV4_ENABLE_JIT_CACHE=1
XDG_GREETER_DATA_DIR=/var/lib/lightdm-data/phablet
SSH_AUTH_SOCK=/tmp/ssh-c6x2sUdjapqR/agent.2851
SHELL=/bin/bash
GDMSESSION=ubuntu-touch
NATIVE_ORIENTATION=Portrait
UPSTART_SESSION=unix:abstract=/com/ubuntu/upstart-session/32011/2851
ASEC_MOUNTPOINT=/mnt/asec
QT_IM_MODULE=maliitphablet
XDG_VTNR=1
PWD=/home/phablet
QT_QPA_PLATFORM=wayland
ANDROID_PROPERTY_WORKSPACE=8,49152
XDG_CONFIG_DIRS=/etc/xdg/xdg-ubuntu-touch:/etc/xdg/xdg-ubuntu-touch:/etc/xdg
XDG_DATA_DIRS=/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera:/usr/share/ubuntu-touch:/usr/share/ubuntu-touch:/usr/local/share/:/usr/share/:/custom/usr/share/
MIR_SERVER_NAME=session-0
ANDROID_BOOTLOGO=1
ANDROID_CACHE=/cache
PULSE_SCRIPT=/etc/pulse/touch.pa
PULSE_CLIENTCONFIG=/etc/pulse/touch-client.conf
GNOME_KEYRING_CONTROL=
PULSE_PROP=media.role=multimedia
GNOME_KEYRING_PID=
PULSE_PLAYBACK_CORK_STALLED=1
UBUNTU_APP_LAUNCH_XMIR_PATH=/usr/bin/libertine-xmir
LIMA_DEBUG=notiling
MIR_SOCKET=/run/user/32011/mir_socket
MIR_SERVER_PROMPT_FILE=1
QML_NO_TOUCH_COMPRESSION=1
QT_ENABLE_GLYPH_CACHE_WORKAROUND=1
UBUNTU_PLATFORM_API_BACKEND=desktop_mirclient
UBUNTU_APP_LAUNCH_ARCH=aarch64-linux-gnu
UBUNTU_APPLICATION_ISOLATION=1
XDG_CACHE_HOME=/home/phablet/.cache
XDG_CONFIG_HOME=/home/phablet/.config
XDG_DATA_HOME=/home/phablet/.local/share
TMPDIR=/run/user/32011/confined/com.ubuntu.camera
__GL_SHADER_DISK_CACHE_PATH=/home/phablet/.cache/com.ubuntu.camera
APP_DIR=/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera
APP_DESKTOP_FILE_PATH=/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/camera-app.desktop
APP_XMIR_ENABLE=0
APP_EXEC=./camera-app %u
APP_ID=com.ubuntu.camera_camera_3.1.3
APP_LAUNCHER_PID=3355
UPSTART_JOB=application-click
UPSTART_INSTANCE=com.ubuntu.camera_camera_3.1.3
LD_LIBRARY_PATH=/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/lib/aarch64-linux-gnu:/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.camera/lib
DCONF_PROFILE=/tmp/camera-dconf
GTK_DEBUG=actions,builder,geometry,icontheme,keybindings,misc,modules,plugsocket,pixel-cache,printing,size-request,text,tree
Create app...
Connect app...
Run app...

(camera-app:11330): dconf-WARNING **: unable to open named profile (/tmp/camera-dconf): using the null configuration.

** (camera-app:11330): WARNING **: Error retrieving accessibility bus address: org.freedesktop.DBus.Error.ServiceUnknown: The name org.a11y.Bus was not provided by any .service files
App activated
Create window...

(camera-app:11330): Gtk-WARNING **: Failed to parse /etc/gtk-3.0/settings.ini: Permission denied
gtk-modules setting changed to: (null)
Show window...
[0xb9c62a0] GtkApplicationWindow	height for width: -1 is minimum 52 and natural: 252 (hit cache: no)
[0xb9c62a0] GtkApplicationWindow	width for height: -1 is minimum 52 and natural: 200 (hit cache: no)
[0xb9c62a0] GtkApplicationWindow	width for height: -1 is minimum 52 and natural: 200 (hit cache: yes)
[0xb9c62a0] GtkApplicationWindow	height for width: -1 is minimum 52 and natural: 252 (hit cache: yes)
gtk_widget_size_allocate:   GtkApplicationWindow 0 0 200 252
[0xb9c62a0] GtkApplicationWindow	height for width: -1 is minimum 52 and natural: 252 (hit cache: yes)
[0xb9c62a0] GtkApplicationWindow	width for height: -1 is minimum 52 and natural: 200 (hit cache: yes)
[0xba52170] GtkLabel	height for width: -1 is minimum 18 and natural: 18 (hit cache: no)
[0xba291c0] GtkBox	height for width: -1 is minimum 18 and natural: 18 (hit cache: no)
[0xba0b190] GtkHeaderBar	height for width: -1 is minimum 18 and natural: 18 (hit cache: no)
[0xb9c62a0] GtkApplicationWindow	height for width: -1 is minimum 70 and natural: 270 (hit cache: no)
[0xba52170] GtkLabel	width for height: -1 is minimum 50 and natural: 50 (hit cache: no)
[0xba291c0] GtkBox	width for height: -1 is minimum 50 and natural: 50 (hit cache: no)
[0xba0b190] GtkHeaderBar	width for height: -1 is minimum 64 and natural: 64 (hit cache: no)
[0xb9c62a0] GtkApplicationWindow	width for height: -1 is minimum 116 and natural: 200 (hit cache: no)
[0xb9c62a0] GtkApplicationWindow	width for height: -1 is minimum 116 and natural: 200 (hit cache: yes)
[0xb9c62a0] GtkApplicationWindow	height for width: -1 is minimum 70 and natural: 270 (hit cache: yes)
gtk_widget_size_allocate:   GtkApplicationWindow 0 0 252 304
[0xb9c62a0] GtkApplicationWindow	height for width: -1 is minimum 70 and natural: 270 (hit cache: yes)
[0xb9c62a0] GtkApplicationWindow	width for height: -1 is minimum 116 and natural: 200 (hit cache: yes)
[0xba0b190] GtkHeaderBar	height for width: -1 is minimum 18 and natural: 18 (hit cache: yes)
gtk_widget_size_allocate:     GtkHeaderBar 26 23 200 18
[0xba0b190] GtkHeaderBar	height for width: -1 is minimum 18 and natural: 18 (hit cache: yes)
[0xba0b190] GtkHeaderBar	width for height: -1 is minimum 64 and natural: 64 (hit cache: yes)
[0xba52490] GtkLabel	width for height: -1 is minimum 74 and natural: 122 (hit cache: no)
[0xba29300] GtkBox	width for height: -1 is minimum 74 and natural: 122 (hit cache: no)
gtk_widget_size_allocate:       GtkBox 65 23 122 18
[0xba52490] GtkLabel	height for width: -1 is minimum 18 and natural: 18 (hit cache: no)
[0xba29300] GtkBox	height for width: -1 is minimum 18 and natural: 18 (hit cache: no)
[0xba29300] GtkBox	width for height: -1 is minimum 74 and natural: 122 (hit cache: yes)
[0xba52490] GtkLabel	height for width: -1 is minimum 18 and natural: 18 (hit cache: yes)
gtk_widget_size_allocate:         GtkLabel 65 23 122 18
[0xba52490] GtkLabel	height for width: -1 is minimum 18 and natural: 18 (hit cache: yes)
[0xba52490] GtkLabel	width for height: -1 is minimum 74 and natural: 122 (hit cache: yes)
gtk_widget_set_clip:              GtkLabel 65 23 122 18
gtk_widget_set_clip:            GtkBox 65 23 122 18
gtk_widget_set_clip:          GtkHeaderBar 26 23 200 18
Error: GDBus.Error:org.freedesktop.DBus.Error.ServiceUnknown: The name com.canonical.PropertyService was not provided by any .service files
*/
