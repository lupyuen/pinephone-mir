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
        ./strace \
            -s 1024 \
            filemanager
        ```

## Debug `gtk` app with GDB

To debug `gtk` app with GDB...

```bash
sudo bash

mount -o remount,rw /

apt install gdb

cd /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
```

Change `run.sh` to...

```
#!/bin/bash
gdb \
    -ex="r" \
    -ex="bt" \
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

## Wayland Messages for `gtk` App

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

Wayland Server returns a list of Registry Objects defined in the server: `wl_drm`, `qt_windowmanager`, `wl_compositor`, `wl_subcompositor`, `wl_seat`, `wl_output`, `wl_data_device_manager`, `wl_shell`, `zxdg_shell_v6`, `xdg_wm_base` and `wl_shm`

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

Wayland Server returns `Fake manufacturer`, `Fake model` and `seat0`

The `gtk` app crashes shortly after this.

## Wayland Messages for `egl` App

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

Wayland Server seems to return a valid response to `egl`.

Followed by these Wayland Messages, which will render the yellow rectangle ([See the source code](egl.c))

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\3\0\0\0\0\0\f\0\5\0\0\0\4\0\0\0\0\0\20\0\6\0\0\0\5\0\0\0\6\0\0\0\3\0\10\0\3\0\0\0\1\0\f\0\7\0\0\0\7\0\0\0\1\0\30\0\0\0\0\0\0\0\0\0\340\1\0\0h\1\0\0\5\0\0\0\4\0\f\0\7\0\0\0\1\0\0\0\1\0\f\0\10\0\0\0\1\0\0\0\0\0\f\0\t\0\0\0", 108}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 108

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\10\0\0\0\0\0\34\0\1\0\0\0\7\0\0\0
wl_drm\0\0\2\0\0\0\10\0\0\0\0\0(\0\2\0\0\0\21\0\0\0
qt_windowmanager\0\0\0\0\1\0\0\0\10\0\0\0\0\0$\0\3\0\0\0\16\0\0\0
wl_compositor\0\0\0\4\0\0\0\10\0\0\0\0\0(\0\4\0\0\0\21\0\0\0
wl_subcompositor\0\0\0\0\1\0\0\0\10\0\0\0\0\0\34\0\5\0\0\0\10\0\0\0
wl_seat\0\6\0\0\0\10\0\0\0\0\0 \0\6\0\0\0\n\0\0\0
wl_output\0\0\0\3\0\0\0\10\0\0\0\0\0,\0\7\0\0\0\27\0\0\0
wl_data_device_manager\0\0\3\0\0\0\10\0\0\0\0\0 \0\10\0\0\0\t\0\0\0
wl_shell\0\0\0\0\1\0\0\0\10\0\0\0\0\0$\0\t\0\0\0\16\0\0\0
zxdg_shell_v6\0\0\0\1\0\0\0\10\0\0\0\0\0 \0\n\0\0\0\f\0\0\0
xdg_wm_base\0\1\0\0\0\10\0\0\0\0\0\34\0\v\0\0\0\7\0\0\0
wl_shm\0\0\1\0\0\0\t\0\0\0\0\0\f\0009\0\0\0\1\0\0\0\1\0\f\0\t\0\0\0", 3696}, {"", 400}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 400
```

Wayland Server returns the list of Registry Objects defined in the server (same as above)

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\10\0\0\0\0\0 \0\1\0\0\0\7\0\0\0
wl_drm\0\0\2\0\0\0\n\0\0\0\1\0\0\0\0\0\f\0\t\0\0\0", 44}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 44
```

`egl` app connects to `wl_drm`, the Wayland Direct Rendering Manager.

```
recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\n\0\0\0\0\0\34\0\17\0\0\0
/dev/dri/card1\0\0\n\0\0\0\1\0\f\0
AR24\n\0\0\0\1\0\f\0
XR24\n\0\0\0\1\0\f\0
RG16\n\0\0\0\1\0\f\0
YUV9\n\0\0\0\1\0\f\0
YU11\n\0\0\0\1\0\f\0
YU12\n\0\0\0\1\0\f\0
YU16\n\0\0\0\1\0\f\0
YU24\n\0\0\0\1\0\f\0
NV12\n\0\0\0\1\0\f\0
NV16\n\0\0\0\1\0\f\0
YUYV\n\0\0\0\3\0\f\0\1\0\0\0\t\0\0\0\0\0\f\0009\0\0\0\1\0\0\0\1\0\f\0\t\0\0\0", 3296}, {"", 800}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 196
```

Wayland Server returns the Linux Display Device `/dev/dri/card1` and the supported colour encoding schemes.

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\n\0\0\0\0\0\f\0\4\0\0\0\1\0\0\0\0\0\f\0\t\0\0\0", 24}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 24

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\n\0\0\0\2\0\10\0\t\0\0\0\0\0\f\0009\0\0\0\1\0\0\0\1\0\f\0\t\0\0\0", 3100}, {"", 996}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 32

sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\5\0\0\0\3\0\f\0\t\0\0\0\n\0\0\0\3\0000\0\v\0\0\0\340\1\0\0h\1\0\0
XR24\0\0\0\0\200\7\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\5\0\0\0\1\0\24\0\v\0\0\0\0\0\0\0\0\0\0\0\5\0\0\0\2\0\30\0\0\0\0\0\0\0\0\0\377\377\377\177\377\377\377\177\5\0\0\0\6\0\10\0", 112}], 
msg_controllen=20, [{cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, [9]}], msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 112
```

`egl` app selects the `XR24` colour encoding.

```
recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\6\0\0\0\1\0\24\0\0\0\0\0\320\2\0\0v\5\0\0", 3068}, {"", 1028}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 20

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\t\0\0\0\0\0\f\0\0\0\0\0\1\0\0\0\1\0\f\0\t\0\0\0", 3048}, {"", 1048}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 24
```

## Wayland Messages for File Manager

Filter Wayland Messages for File Manager...

```bash
grep "msg(" filemanager-strace.log >filemanager-msg.log
```

Wayland Messages for File Manager seem to be similar to `egl` above. It's a Qt app, so it connects to `qt_windowmanager` as well.

Here are the Wayland Messages for File Manager: [`logs/filemanager-msg.log`](logs/filemanager-msg.log)

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\2\0\0\0\0\0,\0\2\0\0\0\21\0\0\0
qt_windowmanager\0\0\0\0\1\0\0\0\4\0\0\0\2\0\0\0\0\0(\0\3\0\0\0\16\0\0\0
wl_compositor\0\0\0\3\0\0\0\5\0\0\0\2\0\0\0\0\0,\0\4\0\0\0\21\0\0\0
wl_subcompositor\0\0\0\0\1\0\0\0\6\0\0\0\2\0\0\0\0\0 \0\5\0\0\0\10\0\0\0
wl_seat\0\4\0\0\0\7\0\0\0\2\0\0\0\0\0$\0\6\0\0\0\n\0\0\0
wl_output\0\0\0\2\0\0\0\10\0\0\0\1\0\0\0\0\0\f\0\t\0\0\0\2\0\0\0\0\0000\0\7\0\0\0\27\0\0\0
wl_data_device_manager\0\0\1\0\0\0\n\0\0\0\n\0\0\0\1\0\20\0\v\0\0\0\7\0\0\0\2\0\0\0\0\0 \0\v\0\0\0\7\0\0\0
wl_shm\0\0\1\0\0\0\f\0\0\0", 304}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 304
```

File Manager connects to `qt_windowmanager` (because it's a Qt app), `wl_compositor`, `wl_subcompositor`, `wl_seat`, `wl_output`, `wl_data_device_manager` and `wl_shm`

```
recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\7\0\0\0\0\0\f\0\7\0\0\0\7\0\0\0\1\0\24\0\6\0\0\0
seat0\0\0\0\10\0\0\0\0\0H\0\0\0\0\0\0\0\0\0D\0\0\0\210\0\0\0\0\0\0\0\22\0\0\0
Fake manufacturer\0\0\0\v\0\0\0
Fake model\0\0\0\0\0\0\10\0\0\0\1\0\30\0\3\0\0\0\320\2\0\0\240\5\0\0)\2\0\0\10\0\0\0\3\0\f\0\1\0\0\0\10\0\0\0\2\0\10\0\t\0\0\0\0\0\f\0?\0\0\0\1\0\0\0\1\0\f\0\t\0\0\0\f\0\0\0\0\0\f\0\0\0\0\0\f\0\0\0\0\0\f\0\1\0\0\0", 3696}, {"", 400}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 196
```

Wayland Server also returns `seat0`, `Fake manufacturer`, `Fake model` to File Manager.

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\7\0\0\0\1\0\f\0\3\0\0\0\7\0\0\0\0\0\f\0\r\0\0\0\5\0\0\0\0\0\f\0\16\0\0\0\7\0\0\0\2\0\f\0\17\0\0\0\f\0\0\0\0\0\20\0\t\0\0\0\0\20\0\0\t\0\0\0\2\0\f\0\0000\0\0\t\0\0\0\2\0\f\0\0p\0\0\t\0\0\0\2\0\f\0\0\360\0\0\t\0\0\0\2\0\f\0\0\360\1\0\t\0\0\0\2\0\f\0\0\360\3\0\t\0\0\0\2\0\f\0\0\360\7\0\t\0\0\0\2\0\f\0\0\360\17\0\1\0\0\0\1\0\f\0\20\0\0\0\1\0\0\0\0\0\f\0\21\0\0\0", 172}], 
msg_controllen=20, [{cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, [5]}], msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 172

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\3\0\0\0\0\0\20\0\1\0\0\0%\260\0\0\3\0\0\0\5\0\20\0\36\0\0\0\310\0\0\0\20\0\0\0\0\0\34\0\1\0\0\0\7\0\0\0
wl_drm\0\0\2\0\0\0\20\0\0\0\0\0(\0\2\0\0\0\21\0\0\0
qt_windowmanager\0\0\0\0\1\0\0\0\20\0\0\0\0\0$\0\3\0\0\0\16\0\0\0
wl_compositor\0\0\0\4\0\0\0\20\0\0\0\0\0(\0\4\0\0\0\21\0\0\0
wl_subcompositor\0\0\0\0\1\0\0\0\20\0\0\0\0\0\34\0\5\0\0\0\10\0\0\0
wl_seat\0\6\0\0\0\20\0\0\0\0\0 \0\6\0\0\0\n\0\0\0
wl_output\0\0\0\3\0\0\0\20\0\0\0\0\0,\0\7\0\0\0\27\0\0\0
wl_data_device_manager\0\0\3\0\0\0\20\0\0\0\0\0 \0\10\0\0\0\t\0\0\0
wl_shell\0\0\0\0\1\0\0\0\20\0\0\0\0\0$\0\t\0\0\0\16\0\0\0
zxdg_shell_v6\0\0\0\1\0\0\0\20\0\0\0\0\0 \0\n\0\0\0\f\0\0\0
xdg_wm_base\0\1\0\0\0\20\0\0\0\0\0\34\0\v\0\0\0\7\0\0\0
wl_shm\0\0\1\0\0\0\21\0\0\0\0\0\f\0?\0\0\0\1\0\0\0\1\0\f\0\21\0\0\0", 3500}, {"", 596}], 
msg_controllen=24, [{cmsg_len=20, cmsg_level=SOL_SOCKET, cmsg_type=SCM_RIGHTS, [5]}], msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 432
```

Wayland Server returns the list of Registry Objects defined in the server (same as above)

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\20\0\0\0\0\0 \0\1\0\0\0\7\0\0\0
wl_drm\0\0\2\0\0\0\22\0\0\0\1\0\0\0\0\0\f\0\21\0\0\0", 44}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 44
```

File Manager app connects to `wl_drm`, the Wayland Direct Rendering Manager.

```
recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\22\0\0\0\0\0\34\0\17\0\0\0
/dev/dri/card1\0\0\22\0\0\0\1\0\f\0
AR24\22\0\0\0\1\0\f\0
XR24\22\0\0\0\1\0\f\0
RG16\22\0\0\0\1\0\f\0
YUV9\22\0\0\0\1\0\f\0
YU11\22\0\0\0\1\0\f\0
YU12\22\0\0\0\1\0\f\0
YU16\22\0\0\0\1\0\f\0
YU24\22\0\0\0\1\0\f\0
NV12\22\0\0\0\1\0\f\0
NV16\22\0\0\0\1\0\f\0
YUYV\22\0\0\0\3\0\f\0\1\0\0\0\21\0\0\0\0\0\f\0?\0\0\0\1\0\0\0\1\0\f\0\21\0\0\0", 3068}, {"", 1028}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 196
```

Wayland Server returns the Linux Display Device `/dev/dri/card1` and the supported colour encoding schemes.

```
sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\22\0\0\0\0\0\f\0\4\0\0\0\1\0\0\0\0\0\f\0\21\0\0\0", 24}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 24

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\22\0\0\0\2\0\10\0\21\0\0\0\0\0\f\0?\0\0\0\1\0\0\0\1\0\f\0\21\0\0\0", 2872}, {"", 1224}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 32

sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\5\0\0\0\0\0\f\0\21\0\0\0\2\0\0\0\0\0(\0\t\0\0\0\16\0\0\0
zxdg_shell_v6\0\0\0\1\0\0\0\23\0\0\0\23\0\0\0\2\0\20\0\24\0\0\0\21\0\0\0\24\0\0\0\1\0\f\0\25\0\0\0\25\0\0\0\2\0$\0\27\0\0\0
com.ubuntu.filemanager\0\0\25\0\0\0\3\0000\0#\0\0\0
filemanager.ubuntu.com.filemanager\0\0\21\0\0\0\10\0\f\0\1\0\0\0\21\0\0\0\7\0\f\0\0\0\0\0\21\0\0\0\6\0\10\0", 196}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 196
```

File Manager connects to `zxdg_shell_v6`. More about this...

1. [`xdg-shell` in "Writing Wayland Clients"](https://bugaevc.gitbooks.io/writing-wayland-clients/beyond-the-black-square/xdg-shell.html)

1. [`xdg-shell` in "The Wayland Book"](https://wayland-book.com/xdg-shell-in-depth.html)

```
recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\25\0\0\0\0\0\24\0\0\0\0\0\0\0\0\0\0\0\0\0\24\0\0\0\0\0\f\0@\0\0\0\21\0\0\0\0\0\f\0\10\0\0\0\3\0\0\0\1\0\24\0A\0\0\0\21\0\0\0\0\0\0\0\25\0\0\0\0\0\30\0\0\0\0\0\0\0\0\0\4\0\0\0\4\0\0\0\24\0\0\0\0\0\f\0B\0\0\0", 2840}, {"", 1256}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 100

sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\24\0\0\0\4\0\f\0@\0\0\0\1\0\0\0\0\0\f\0\26\0\0\0\24\0\0\0\4\0\f\0B\0\0\0", 36}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 36

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\26\0\0\0\0\0\f\0B\0\0\0\1\0\0\0\1\0\f\0\26\0\0\0\25\0\0\0\0\0\30\0\320\2\0\0v\5\0\0\4\0\0\0\4\0\0\0\24\0\0\0\0\0\f\0C\0\0\0", 2740}, {"", 1356}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 60

sendmsg(3, {msg_name(0)=NULL, msg_iov(1)=[{"\24\0\0\0\4\0\f\0C\0\0\0", 12}], 
msg_controllen=0, msg_flags=0}, MSG_DONTWAIT|MSG_NOSIGNAL) = 12

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\1\0\0\0\1\0\f\0\30\0\0\0\26\0\0\0\0\0\10\0\27\0\0\0\0\0\f\0\0\0\0\0\1\0\0\0\1\0\f\0\27\0\0\0", 2596}, {"", 1500}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 44

recvmsg(3, {msg_name(0)=NULL, msg_iov(2)=[{"\31\0\0\0\0\0\10\0\27\0\0\0\0\0\f\0\0\0\0\0\1\0\0\0\1\0\f\0\27\0\0\0", 2552}, {"", 1544}], 
msg_controllen=0, msg_flags=MSG_CMSG_CLOEXEC}, MSG_DONTWAIT|MSG_CMSG_CLOEXEC) = 32
```

## Build GTK Toolkit Library on PinePhone with Ubuntu Touch

_Why build GTK Library?_

Because according to `gdb`, our GTK App crashed in `libgdk-3.so`, a GTK Shared Library which has no debug symbols.

So by building GTK Library ourselves and linking our app to the GTK Static Library, we might find out why it crashed.

```bash
# Make system folders writeable
sudo mount -o remount,rw /

# Install gobject-introspection Library. Must be installed before pip3 because it messes up Python.
sudo apt install libgirepository1.0-dev

# Remove outdated glib-compile-resources, it fails the sassc build with error: The "dependencies" argument of gnome.compile_resources() can not be used with the current version of glib-compile-resources due to <https://bugzilla.gnome.org/show_bug.cgi?id=774368>"
sudo mv /usr/bin/glib-compile-resources /usr/bin/glib-compile-resources-old

# Fix python3 as default for python. Needed by meson and ninja.
sudo mv /usr/bin/python /usr/bin/python.old
sudo ln -s /usr/bin/python3 /usr/bin/python
python --version
# Should show "Python 3.5.2"

# Install pip3
cd ~
curl https://bootstrap.pypa.io/get-pip.py -o get-pip.py
sudo -H python get-pip.py

# Install Meson
sudo -H pip3 install meson
sudo ln -s /usr/local/bin/meson /usr/bin

# Install Ninja
cd ~
git clone https://github.com/ninja-build/ninja
cd ninja
./configure.py --bootstrap
sudo cp ninja /usr/bin

# Build GLib (to update glib-compile-resources)
cd ~
git clone https://gitlab.gnome.org/GNOME/glib
cd glib
meson _build
ninja -C _build

# Build GTK
cd ~
git clone https://gitlab.gnome.org/GNOME/gtk.git
cd gtk
meson wrap promote subprojects/glib/subprojects/libffi.wrap
meson wrap promote subprojects/pango/subprojects/fribidi.wrap
meson wrap promote subprojects/pango/subprojects/harfbuzz.wrap
meson _build .
```

Stops with this error...

```
|Executing subproject sassc method meson 
|
|Project name: sassc
|Project version: 3.5.0.99
|C compiler for the host machine: cc (gcc 5.4.0 "cc (Ubuntu/Linaro 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609")
|C linker for the host machine: cc ld.bfd 2.26.1
|Configuring sassc_version.h using configuration
|Run-time dependency libsass found: NO (tried pkgconfig and cmake)
|Looking for a fallback subproject for the dependency libsass
|
||Executing subproject libsass method meson 
||
||Project name: libsass
||Project version: 3.5.5.99
||C compiler for the host machine: cc (gcc 5.4.0 "cc (Ubuntu/Linaro 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609")
||C linker for the host machine: cc ld.bfd 2.26.1
||C++ compiler for the host machine: c++ (gcc 5.4.0 "c++ (Ubuntu/Linaro 5.4.0-6ubuntu1~16.04.12) 5.4.0 20160609")
||C++ linker for the host machine: c++ ld.bfd 2.26.1
||Compiler for C++ supports arguments -Wno-non-virtual-dtor -Wnon-virtual-dtor: YES 
||Configuring version.h using configuration
||Library dl found: YES
||Configuring libsass.pc using configuration
||Build targets in project: 800
||Subproject libsass finished.
|
|Dependency libsass from subproject subprojects/libsass found: YES 3.5.5.99
|Build targets in project: 801
|Subproject sassc finished.

Program sassc found: YES (overriden)

gtk/meson.build:845:0: ERROR: The "dependencies" argument of gnome.compile_resources() can not
be used with the current version of glib-compile-resources due to
<https://bugzilla.gnome.org/show_bug.cgi?id=774368>
```

## Wayland Compositor for Ubuntu Touch: `unity-system-compositor`

TODO

```
$ ps -aux
unity-system-compositor \
    --enable-num-framebuffers-quirk=true \
    --disable-overlays=false \
    --console-provider=vt \
    --spinner=/usr/bin/unity-system-compositor-spinner \
    --file /run/mir_socket \
    --from-dm-fd 11 \
    --to-dm-fd 14 --vt 1
```
