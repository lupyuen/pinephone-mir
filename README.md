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
