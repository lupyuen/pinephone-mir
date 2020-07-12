# pinephone-mir
Experiments with Wayland and Mir display server on PinePhone with Ubuntu Touch

## How to run `strace` on the `gtk` app

Assume that we have built the `gtk` app by running [`gtk.sh`](gtk.sh)

We shall replace the File Manager app by `strace gtk` because it has no AppArmor restrictions (Unconfined) according to `/usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/filemanager.apparmor`...

```
{
    "policy_version": 16.04,
    "template": "unconfined",
    "policy_groups": []
}
```

Connect to PinePhone over SSH and run these commands...

```bash
sudo bash

mount -o remount,rw /

cd /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager

# Back up the desktop file
cp com.ubuntu.filemanager.desktop com.ubuntu.filemanager.desktop.old

nano com.ubuntu.filemanager.desktop 
```

Change the `Exec` line from...

```
Exec=filemanager
```

To...

```
Exec=./run.sh
```

Save and exit `nano`

Create `run.sh` with `nano`, containing:

```bash
#!/bin/bash
./strace \
    -s 1024 \
    ./gtk
```

Copy `strace`, `gtk` and set ownership...

```bash
cp /usr/bin/strace .

cp /home/phablet/pinephone-mir/gtk .

chown clickpkg:clickpkg strace gtk run.sh

chmod a+x run.sh
```

We should see the files with permissions set as follows...

```bash
$ ls -l
total 784
-rw-r--r--  1 clickpkg clickpkg   3004 Jul 12 04:22 com.ubuntu.filemanager.desktop
-rw-r--r--  1 root     root       3007 Jul 12 04:16 com.ubuntu.filemanager.desktop.old
-rw-r--r--  1 clickpkg clickpkg    235 Jan 16 06:00 content-hub.json
-rw-r--r--  1 clickpkg clickpkg     87 Jan 16 06:00 filemanager.apparmor
-rw-r--r--  1 clickpkg clickpkg    874 Jan 16 06:00 filemanager.svg
-rwxr-xr-x  1 clickpkg clickpkg  17664 Jul 12 04:20 gtk
drwxr-xr-x  3 clickpkg clickpkg   4096 May  6 02:04 lib
drwxr-xr-x 11 clickpkg clickpkg   4096 May  6 02:04 qml
-rwxr-xr-x  1 clickpkg clickpkg    103 Jul 12 12:42 run.sh
drwxr-xr-x  3 clickpkg clickpkg   4096 May  6 02:04 share
-rwxr-xr-x  1 clickpkg clickpkg 739488 Jul 12 04:19 strace
drwxr-xr-x  3 clickpkg clickpkg   4096 May  6 02:04 usr
```

Tap on File Manager icon on PinePhone to start the `strace`

Check the `strace` log in...

`/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log`

## `strace` Logs

1. `gtk` App: Simple GTK App (not working)

    - [Source Code](gtk.c)

    - [Build Script](gtk.sh)

    - [`strace` Log](logs/gtk-strace.log)

    - Crashes with a Segmentation Fault. Now investigating with GDB. (See next section)

1. `egl` App: Wayland EGL App (draws a yellow rectangle)

    - [Source Code](egl.c)

    - [Build Script](egl.sh)

    - [`strace` Log](logs/egl-strace.log)

    - Runs OK, renders a yellow rectangle

1. File Manager App

    - [Source Code](https://gitlab.com/ubports/apps/filemanager-app/-/tree/master)

    - [`strace` Log](logs/filemanager-strace.log)

    - `strace` Log was captured by changing `run.sh` to...

        ```
        #!/bin/bash
        ./strace filemanager
        ```

## Debug `gtk` app with GDB

To debug `gtk` app with GDB...

```bash
sudo bash

apt install gdb

mount -o remount,rw /

cd /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
```

Change `run.sh` to...

```
#!/bin/bash
gdb \
    -ex=r \
    -ex=bt \
    --args ./gtk
```

Tap on File Manager icon on PinePhone to start `gdb` debugger on `gtk` app.

Log shows exception and backtrace...

`/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log`

```
...
gtk_widget_set_clip:              GtkLabel 89 23 74 18
gtk_widget_set_clip:            GtkBox 89 23 74 18
gtk_widget_set_clip:          GtkHeaderBar 26 23 200 18

Thread 1 "gtk" received signal SIGSEGV, Segmentation fault.
0x0000fffff6d21f8c in wl_proxy_marshal_constructor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-client.so.0
#0  0x0000fffff6d21f8c in wl_proxy_marshal_constructor ()
   from /usr/lib/aarch64-linux-gnu/libwayland-client.so.0
#1  0x0000fffff75c2d44 in ?? () from /usr/lib/aarch64-linux-gnu/libgdk-3.so.0
---Type <return> to continue, or q <return> to quit---#2  0x000000000045d450 in ?? ()
Backtrace stopped: previous frame inner to this frame (corrupt stack?)
(gdb) quit
A debugging session is active.

        Inferior 1 [process 30537] will be killed.

Quit anyway? (y or n) [answered Y; input not from terminal]
```

So our simple `gtk` app crashed in `wl_proxy_marshal_constructor()` (from `libwayland-client.so.0`), which is called by `libgdk-3.so.0`

Interesting.

## Compare `gtk` vs `egl` logs

Why does `egl` render OK but not `gtk`?

We compare `gtk` and `egl` logs by filtering for Wayland Messages...

```bash
grep "msg(" gtk-strace.log >gtk-msg.log
grep "msg(" egl-strace.log >egl-msg.log
```

Here are the Wayland Messages for `gtk` in [`logs/gtk-msg.log`](logs/gtk-msg.log):

```
sendmsg(7, {msg_name(0)=NULL, msg_iov(1)=[{"\1\0\0\0\1\0\f\0\2\0\0\0\1\0\0\0\0\0\f\0\3\0\0\0", 24}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 24
```

`gtk` app connects to the Wayland Server socket.

```
recvmsg(7, {msg_name(0)=NULL, msg_iov(1)=[{"\2\0\0\0\0\0\34\0\1\0\0\0\7\0\0\0
wl_drm\0\0\2\0\0\0\2\0\0\0\0\0(\0\2\0\0\0\21\0\0\0
qt_windowmanager\0\0\0\0\1\0\0\0\2\0\0\0\0\0$\0\3\0\0\0\16\0\0\0
wl_compositor\0\0\0\4\0\0\0\2\0\0\0\0\0(\0\4\0\0\0\21\0\0\0
wl_subcompositor\0\0\0\0\1\0\0\0\2\0\0\0\0\0\34\0\5\0\0\0\10\0\0\0
wl_seat\0\6\0\0\0\2\0\0\0\0\0 \0\6\0\0\0\n\0\0\0
wl_output\0\0\0\3\0\0\0\2\0\0\0\0\0,\0\7\0\0\0\27\0\0\0
wl_data_device_manager\0\0\3\0\0\0\2\0\0\0\0\0 \0\10\0\0\0\t\0\0\0
wl_shell\0\0\0\0\1\0\0\0\2\0\0\0\0\0$\0\t\0\0\0\16\0\0\0
zxdg_shell_v6\0\0\0\1\0\0\0\2\0\0\0\0\0 \0\n\0\0\0\f\0\0\0
xdg_wm_base\0\1\0\0\0\2\0\0\0\0\0\34\0\v\0\0\0\7\0\0\0
wl_shm\0\0\1\0\0\0\3\0\0\0\0\0\f\0009\0\0\0\1\0\0\0\1\0\f\0\3\0\0\0", 4096}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 400
```

Wayland Server returns a list of Registry Objects defined in the server: `wl_drm`, `qt_windowmanager`, ..., `wl_shm`

```
sendmsg(7, {msg_name(0)=NULL, msg_iov(1)=[{"\2\0\0\0\0\0(\0\3\0\0\0\16\0\0\0
wl_compositor\0\0\0\3\0\0\0\4\0\0\0\2\0\0\0\0\0,\0\4\0\0\0\21\0\0\0
wl_subcompositor\0\0\0\0\1\0\0\0\5\0\0\0\2\0\0\0\0\0$\0\6\0\0\0\n\0\0\0
wl_output\0\0\0\2\0\0\0\6\0\0\0\1\0\0\0\0\0\f\0\7\0\0\0\2\0\0\0\0\0000\0\7\0\0\0\27\0\0\0
wl_data_device_manager\0\0\1\0\0\0\10\0\0\0\2\0\0\0\0\0 \0\5\0\0\0\10\0\0\0
wl_seat\0\4\0\0\0\t\0\0\0\4\0\0\0\0\0\f\0\n\0\0\0\n\0\0\0\4\0\f\0\0\0\0\0\n\0\0\0\5\0\f\0\0\0\0\0\10\0\0\0\1\0\20\0\v\0\0\0\t\0\0\0\4\0\0\0\0\0\f\0\f\0\0\0\1\0\0\0\0\0\f\0\r\0\0\0\2\0\0\0\0\0 \0\v\0\0\0\7\0\0\0
wl_shm\0\0\1\0\0\0\16\0\0\0", 320}],
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 320
```

`gtk` app attempts to connect to the Registry Objects `wl_compositor`, `wl_subcompositor`, `wl_output`, `wl_data_device_manager`, `wl_seat` and `wl_shm`

```
recvmsg(7, {msg_name(0)=NULL, msg_iov(2)=[{"\6\0\0\0\0\0H\0\0\0\0\0\0\0\0\0D\0\0\0\210\0\0\0\0\0\0\0\22\0\0\0
Fake manufacturer\0\0\0\v\0\0\0
Fake model\0\0\0\0\0\0\6\0\0\0\1\0\30\0\3\0\0\0\320\2\0\0\240\5\0\0)\2\0\0\6\0\0\0\3\0\f\0\1\0\0\0\6\0\0\0\2\0\10\0\7\0\0\0\0\0\f\0
009\0\0\0\1\0\0\0\1\0\f\0\7\0\0\0\t\0\0\0\0\0\f\0\7\0\0\0\t\0\0\0\1\0\24\0\6\0\0\0
seat0\0\0\0\r\0\0\0\0\0\f\0
009\0\0\0\1\0\0\0\1\0\f\0\r\0\0\0\16\0\0\0\0\0\f\0\0\0\0\0\16\0\0\0\0\0\f\0\1\0\0\0", 3696}, {"", 400}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 220
```

But Wayland Server returns `Fake manufacturer` and `Fake model`. So the requested registry objects were not valid.

Compare with the Wayland Messages for `egl` in [`logs/egl-msg.log`](logs/egl-msg.log):

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\2\0\0\0\0\0(\0\3\0\0\0\16\0\0\0
wl_compositor\0\0\0\1\0\0\0\3\0\0\0\2\0\0\0\0\0$\0\10\0\0\0\t\0\0\0
wl_shell\0\0\0\0\1\0\0\0\4\0\0\0\1\0\0\0\0\0\f\0\5\0\0\0", 88}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 88
```

`egl` app attempts to connect to the Registry Objects `wl_compositor` and `wl_shell`

```
recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\5\0\0\0\0\0\f\0
009\0\0\0\1\0\0\0\1\0\f\0\5\0\0\0", 3720}, {"", 376}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 24
```

Wayland Server seems to return a valid response. No `Fake` response.

## `unity-system-compositor`

TODO

```
unity-system-compositor \
    --enable-num-framebuffers-quirk=true \
    --disable-overlays=false \
    --console-provider=vt \
    --spinner=/usr/bin/unity-system-compositor-spinner \
    --file /run/mir_socket \
    --from-dm-fd 11 \
    --to-dm-fd 14 --vt 1
```
