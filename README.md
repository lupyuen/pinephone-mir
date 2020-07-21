# pinephone-mir
Experiments with Wayland and Mir display server on PinePhone with Ubuntu Touch

Read the article...

[_"Wayland and LVGL on PinePhone with Ubuntu Touch"_](https://lupyuen.github.io/pinetime-rust-mynewt/articles/wayland)

## Build and Install SDL2 with Wayland Support

The `apt install` version of SDL2 doesn't support Wayland, so we need to build SDL2 ourselves.

Connect to PinePhone over SSH and run these commands...

```bash
sudo mount -o remount,rw /
sudo apt install wayland-protocols libxkbcommon-dev
cd ~
git clone https://github.com/lupyuen/SDL-ubuntu-touch
cd SDL-ubuntu-touch
./configure \
    --enable-video-wayland \
    --disable-video-wayland-qt-touch \
    --disable-wayland-shared \
    --disable-video-x11
make
sudo make install
sudo ln -s /usr/local/lib/libSDL2-2.0.so.0 /usr/lib/aarch64-linux-gnu/
sudo ln -s /usr/local/lib/libSDL2.so /usr/lib/aarch64-linux-gnu/
```

## Build SDL2 App on PinePhone with Ubuntu Touch

Check out the sample SDL program [`sdl.c`](sdl.c) and build script [`sdl.sh`](sdl.sh).

To build the `sdl` app on PinePhone via SSH...

```bash
cd ~
git clone https://github.com/lupyuen/pinephone-mir
cd pinephone-mir
gcc \
    -g \
    -o sdl \
    sdl.c \
    -lSDL2 \
    -Wl,-Map=sdl.map
```

This generates an executable named `sdl`

## Run SDL2 App on PinePhone with Ubuntu Touch

For rapid testing, we shall replace the File Manager app by our `sdl` app because File Manager has no AppArmor restrictions (Unconfined).

Connect to PinePhone over SSH and run these commands...

```bash
# Make system folders writeable and go to File Manager folder
sudo mount -o remount,rw /
cd /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager

# Back up the desktop file
sudo cp com.ubuntu.filemanager.desktop com.ubuntu.filemanager.desktop.old
sudo nano com.ubuntu.filemanager.desktop 
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

Check that `~/pinephone-mir/run.sh` contains the following...

```bash
export SDL_VIDEODRIVER=wayland
export SDL_DEBUG=1
./sdl
```

Then enter these commands...

```bash
# Make system folders writeable
sudo mount -o remount,rw /

# Copy app to File Manager folder
cd ~/pinephone-mir
sudo cp sdl /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/sdl
ls -l /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/sdl

# Copy run script to File Manager folder
# TODO: Check that run.sh contains "./sdl"
sudo cp run.sh /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
sudo chown clickpkg:clickpkg /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/run.sh

# Start the File Manager
echo "*** Tap on File Manager icon on PinePhone"

# Monitor the log file
echo >/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
tail -f /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
```

Press `Ctrl-C` to stop the log display. To kill the app...

```bash
pkill sdl
```

## Debug `sdl` app with GDB

To debug `sdl` app with GDB...

```bash
sudo bash
sudo mount -o remount,rw /
sudo apt install gdb
sudo nano /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/run.sh
```

Change the contents of `run.sh` to...

```
#!/bin/bash
export SDL_VIDEODRIVER=wayland
export SDL_DEBUG=1
gdb \
    -ex="r" \
    -ex="bt" \
    --args ./sdl
```

Save the file and exit `nano`.

Tap on File Manager icon on PinePhone to start `gdb` debugger on `sdl` app.

Press Ctrl-C to stop the log. The log is located here...

`/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log`

Copy the log to our machine like this...

```bash
scp -i ~/.ssh/pinebook_rsa phablet@192.168.1.10:/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log .
```

Log shows exception and backtrace...

```
Reading symbols from ./filemanager.ubuntu.com.filemanager...done.
Starting program: /usr/share/click/preinstalled/com.ubuntu.filemanager/0.7.5/filemanager.ubuntu.com.filemanager 
[Thread debugging using libthread_db enabled]
Using host libthread_db library "/lib/aarch64-linux-gnu/libthread_db.so.1".
Init SDL...
[New Thread 0xfffff7b98200 (LWP 13733)]
[3395216.659]  -> wl_display@1.get_registry(new id wl_registry@2)
[3395216.821]  -> wl_display@1.sync(new id wl_callback@3)
[3395256.434] wl_display@1.delete_id(3)
[3395256.669] wl_registry@2.global(1, "wl_drm", 2)
WAYLAND INTERFACE: wl_drm
[3395256.937] wl_registry@2.global(2, "qt_windowmanager", 1)
WAYLAND INTERFACE: qt_windowmanager
[3395257.252] wl_registry@2.global(3, "wl_compositor", 4)
WAYLAND INTERFACE: wl_compositor
[3395258.252]  -> wl_registry@2.bind(3, "wl_compositor", 3, new id [unknown]@4)
[3395258.433] wl_registry@2.global(4, "wl_subcompositor", 1)
WAYLAND INTERFACE: wl_subcompositor
[3395258.545] wl_registry@2.global(5, "wl_seat", 6)
WAYLAND INTERFACE: wl_seat
[3395258.645]  -> wl_registry@2.bind(5, "wl_seat", 5, new id [unknown]@5)
[3395258.848] wl_registry@2.global(6, "wl_output", 3)
WAYLAND INTERFACE: wl_output
[3395259.000]  -> wl_registry@2.bind(6, "wl_output", 2, new id [unknown]@6)
[3395259.112] wl_registry@2.global(7, "wl_data_device_manager", 3)
WAYLAND INTERFACE: wl_data_device_manager
[3395259.199]  -> wl_registry@2.bind(7, "wl_data_device_manager", 3, new id [unknown]@7)
[3395259.339] wl_registry@2.global(8, "wl_shell", 1)
WAYLAND INTERFACE: wl_shell
[3395259.437]  -> wl_registry@2.bind(8, "wl_shell", 1, new id [unknown]@8)
[3395259.598] wl_registry@2.global(9, "zxdg_shell_v6", 1)
WAYLAND INTERFACE: zxdg_shell_v6
[3395259.729]  -> wl_registry@2.bind(9, "zxdg_shell_v6", 1, new id [unknown]@9)
[3395259.928] wl_registry@2.global(10, "xdg_wm_base", 1)
WAYLAND INTERFACE: xdg_wm_base
[3395260.023] wl_registry@2.global(11, "wl_shm", 1)
WAYLAND INTERFACE: wl_shm
[3395260.108]  -> wl_registry@2.bind(11, "wl_shm", 1, new id [unknown]@10)
[3395260.615]  -> wl_shm@10.create_pool(new id wl_shm_pool@11, fd 5, 4096)
[3395262.619]  -> wl_shm_pool@11.resize(12288)
[3395263.695]  -> wl_shm_pool@11.resize(28672)
[3395265.468]  -> wl_shm_pool@11.resize(61440)
[3395276.673]  -> wl_shm_pool@11.resize(126976)
[3395277.196]  -> wl_shm_pool@11.resize(258048)
[3395284.160]  -> wl_shm_pool@11.resize(520192)
[3395300.514]  -> wl_shm_pool@11.resize(1044480)
[3395348.549] wl_callback@3.done(255)
[3395348.697]  -> wl_display@1.sync(new id wl_callback@3)
[3395348.869] wl_seat@5.capabilities(7)
[3395348.920]  -> wl_seat@5.get_pointer(new id wl_pointer@12)
[3395349.019]  -> wl_seat@5.get_touch(new id wl_touch@13)
[3395349.071]  -> wl_seat@5.get_keyboard(new id wl_keyboard@14)
[3395349.117] wl_seat@5.name("seat0")
[3395349.862] wl_display@1.delete_id(3)
[3395349.959] wl_output@6.geometry(0, 0, 68, 136, 0, "Fake manufacturer", "Fake model", 0)
[3395350.071] wl_output@6.mode(3, 720, 1440, 553)
[3395350.213] wl_output@6.scale(1)
[3395350.256] wl_output@6.done()
[3395350.285] wl_callback@3.done(255)
[3395350.362]  -> wl_shm_pool@11.create_buffer(new id wl_buffer@3, 770048, 32, 32, 128, 0)
[3395350.475]  -> wl_compositor@4.create_surface(new id wl_surface@15)
[3395350.536]  -> wl_pointer@12.set_cursor(0, wl_surface@15, 10, 5)
[3395350.583]  -> wl_surface@15.attach(wl_buffer@3, 0, 0)
[3395350.616]  -> wl_surface@15.damage(0, 0, 32, 32)
[3395350.653]  -> wl_surface@15.commit()
Creating window...
[3395355.709]  -> wl_compositor@4.create_surface(new id wl_surface@16)
[3395355.813]  -> zxdg_shell_v6@9.get_xdg_surface(new id zxdg_surface_v6@17, wl_surface@16)
[3395355.853]  -> zxdg_surface_v6@17.get_toplevel(new id zxdg_toplevel_v6@18)
[3395355.882]  -> zxdg_toplevel_v6@18.set_app_id("filemanager.ubuntu.com.filemanager")
[3395355.945]  -> zxdg_toplevel_v6@18.destroy()
[3395355.982]  -> zxdg_surface_v6@17.destroy()
[3395356.009]  -> wl_surface@16.destroy()
filemanager.ubuntu.com.filemanager: sdl.c:24: main: Assertion `window != NULL' failed.

Thread 1 "filemanager.ubu" received signal SIGABRT, Aborted.
0x0000fffff7d5f568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
#0  0x0000fffff7d5f568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
#1  0x0000fffff7d60a20 in abort () from /lib/aarch64-linux-gnu/libc.so.6
#2  0x0000fffff7d58c44 in ?? () from /lib/aarch64-linux-gnu/libc.so.6
#3  0x0000000000400e40 in ?? ()
---Type <return> to continue, or q <return> to quit---Backtrace stopped: previous frame inner to this frame (corrupt stack?)
#0  0x0000fffff7d5f568 in raise () from /lib/aarch64-linux-gnu/libc.so.6
(gdb) quit
```

So SDL2 doesn't work on PinePhone with Ubuntu Touch yet. SDL2 seems to be a problem creating windows with Wayland XDG.

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

Edit `run.sh` with `nano`...

```bash
sudo nano /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/run.sh
```

Change the contents to...

```bash
#!/bin/bash
./strace \
    -s 1024 \
    ./gtk
```

Copy `strace`, `gtk` and set ownership...

```bash
sudo cp /usr/bin/strace .
sudo cp /home/phablet/pinephone-mir/gtk .
sudo chown clickpkg:clickpkg strace gtk run.sh
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
sudo mount -o remount,rw /
sudo apt install gdb
sudo nano /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager/run.sh
```

Change the contents of `run.sh` to...

```
#!/bin/bash
gdb \
    -ex="r" \
    -ex="bt" \
    --args ./gtk
```

Save the file and exit `nano`.

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

# On Arch Linux: 
# sudo pacman -S gobject-introspection

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
sudo cp _build/gio/glib-compile-resources /usr/bin
glib-compile-resources --version
# Should show "2.65.0"

# Build GTK
cd ~
git clone https://gitlab.gnome.org/GNOME/gtk.git
cd gtk
meson _build .
# Will fail with an libffi error

# Promote libffi and rebuild
meson wrap promote subprojects/glib/subprojects/libffi.wrap
meson _build .
# Will fail with a fribidi error

# Promote fribidi and rebuild
meson wrap promote subprojects/pango/subprojects/fribidi.wrap
meson _build .
# Will fail with a harfbuzz error

# Promote harfbuzz and rebuild
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

## What I like about Ubuntu Touch on PinePhone

1. __AppArmor is good__, because iOS and Android have similar apps security

1. __Read-only file system is good__ (system files are read-only by default, user files are read-write). Helps to prevent security holes. (Even PineTime has a read-only Flash ROM)

1. __Why is Qt supported on Ubuntu Touch and not GTK?__ Because building a Linux mobile app requires mobile-friendly widgets. 

    I think Qt has more mobile-friendly widgets, even through the internal plumbing is way too complicated. 
    
    When I get GTK running on Ubuntu Touch, I will face the same problem with widgets. And I have to make GTK widgets look and feel consistent with Qt / Ubuntu Touch widgets.

1. __Older kernel base__ in Ubuntu Touch... I don't do kernel hacking much so it doesn't matter to me. 

    I think for mobiles we only need to support a few common chipsets, so an older kernel is probably fine. 
    
    That explains why Raspberry Pi 4 isn't supported by Ubuntu Touch... The hardware is just too new.

1. The issues I'm struggling with now... Wayland, GTK3, ... are actually really old stuff. Updating the kernel won't help.

1. __Ubuntu Touch is pure Wayland__, none of the legacy X11 stuff. Xwayland is not even there (unless you use the Libertine containers ugh). 

    The pure Wayland environment causes GTK to break, because GTK assumes some minimal X11 support (i.e. Xwayland).

So Ubuntu Touch is not really that bad for PinePhone... It's just painful for building non-Qt apps. 🙂

## Mir Server for Ubuntu Touch

Runs as the `unity-system-compositor` process...

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

Mir Server Log may be found on PinePhone here...

`/home/phablet/.cache/upstart/unity8.log`

Copy the log to our machine like this...

```bash
scp -i ~/.ssh/pinebook_rsa phablet@192.168.1.10:/home/phablet/.cache/upstart/unity8.log .
```

Check the sample log: [`logs/unity8.log`](logs/unity8.log)

```
[2020-07-15:10:58:22.762] Using mirserver qt platform
[2020-07-15:10:58:22.762] Not using nested server, using null mir cursor
[2020-07-15:10:58:23.075] qtmir.screens: ScreensModel::ScreensModel
[2020-07-15 10:58:23.181861] <information> mirserver: Starting
[2020-07-15 10:58:23.272222] < - debug - > mirserver: Using logind for session management
[2020-07-15 10:58:23.276582] <information> mircommon: Loading modules from: /usr/lib/aarch64-linux-gnu/mir/server-platform
[2020-07-15 10:58:23.277579] <information> mircommon: Loading module: /usr/lib/aarch64-linux-gnu/mir/server-platform/graphics-mesa-kms.so.16
[2020-07-15 10:58:23.278121] <information> mircommon: Loading module: /usr/lib/aarch64-linux-gnu/mir/server-platform/graphics-wayland.so.16
[2020-07-15 10:58:23.278840] <information> mircommon: Loading module: /usr/lib/aarch64-linux-gnu/mir/server-platform/server-mesa-x11.so.16
[2020-07-15 10:58:23.279606] <information> mircommon: Loading module: /usr/lib/aarch64-linux-gnu/mir/server-platform/input-evdev.so.7
[2020-07-15 10:58:23.303380] <information> mesa-kms: EGL platform does not support EGL_KHR_platform_gbm extension
[2020-07-15 10:58:23.338281] < -warning- > mesa-kms: Failed to detect whether device /dev/dri/card0 supports KMS, continuing with lower confidence
[2020-07-15 10:58:23.417808] <information> mirserver: Found graphics driver: mir:mesa-kms (version 1.7.2) Support priority: 128
[2020-07-15 10:58:23.418182] <information> mirserver: Found graphics driver: mir:wayland (version 1.7.2) Support priority: 256
[2020-07-15 10:58:23.418449] <information> mirserver: Found graphics driver: mir:mesa-x11 (version 1.7.2) Support priority: 0
[2020-07-15 10:58:23.420831] <information> mirserver: Selected driver: mir:wayland (version 1.7.2)
[2020-07-15:10:58:24.016] qtmir.mir: PromptSessionListener::PromptSessionListener - this= PromptSessionListener(0xffff9812a5e0)
[2020-07-15 10:58:24.018289] <information> mirserver: Cursor disabled
[2020-07-15 10:58:24.057022] <information> mirserver: Selected input driver: mir:wayland (version: 1.7.2)
[2020-07-15:10:58:24.057] qtmir.mir: MirServer created
[2020-07-15:10:58:24.058] qtmir.mir: Command line arguments passed to Qt: ("unity8", "--mode=full-greeter")
[2020-07-15 10:58:24.059135] <information> mirserver: Mir version 1.7.2
[2020-07-15:10:58:24.059] qtmir.screens: QtCompositor::start
[2020-07-15 10:58:24.104796] <information> mirserver: Initial display configuration:
[2020-07-15 10:58:24.105203] <information> mirserver: * Output 0: unknown connected, used
[2020-07-15 10:58:24.105266] <information> mirserver: . |_ Physical size 68x136mm
[2020-07-15 10:58:24.105311] <information> mirserver: . |_ Power is on
[2020-07-15 10:58:24.105347] <information> mirserver: . |_ Current mode 720x1440
[2020-07-15 10:58:24.105389] <information> mirserver: . |_ Preferred mode 720x1440
[2020-07-15 10:58:24.105495] <information> mirserver: . |_ Orientation normal
[2020-07-15 10:58:24.105617] <information> mirserver: . |_ Logical size 720x1440
[2020-07-15 10:58:24.105659] <information> mirserver: . |_ Logical position +0+0
[2020-07-15:10:58:24.105] qtmir.screens: ScreensModel::update
[2020-07-15:10:58:24.106] qtmir.sensor: Screen - nativeOrientation is: Qt::ScreenOrientation(PortraitOrientation)
[2020-07-15:10:58:24.106] qtmir.sensor: Screen - initial currentOrientation is: Qt::ScreenOrientation(PortraitOrientation)
[2020-07-15:10:58:24.107] is not internalDisplay, but enabling anyway
[2020-07-15:10:58:24.107] qtmir.screens: Added Screen with id 0 and geometry QRect(0,0 720x1440)
[2020-07-15:10:58:24.140] qtmir.screens: Screen::setMirDisplayBuffer Screen(0xffff98133960) 0xffff980a0788 0xffff980a0780
[2020-07-15:10:58:24.140] qtmir.screens: =======================================
[2020-07-15:10:58:24.141] qtmir.screens: Screen(0xffff98133960) - id: 0 geometry: QRect(0,0 720x1440) window: 0x0 type: "Unknown" scale: 1.75
[2020-07-15:10:58:24.141] qtmir.screens: =======================================
library "libcutils.so" not found
failed to load bionic libc.so, falling back own property implementation
[2020-07-15:10:58:26.011] QObject::connect: signal not found in qtmir::Wakelock
[2020-07-15:10:58:26.379] file:///usr/share/unity8//Components/Showable.qml:44: Error: Cannot assign to non-existent property "target"
[2020-07-15:10:58:26.395] file:///usr/share/unity8//Components/Showable.qml:43: Error: Cannot assign to non-existent property "target"
[2020-07-15:10:58:26.621] file:///usr/share/unity8//Components/Showable.qml:44: Error: Cannot assign to non-existent property "target"
[2020-07-15:10:58:26.623] file:///usr/share/unity8//Components/Showable.qml:43: Error: Cannot assign to non-existent property "target"
[2020-07-15:10:58:27.213] qtmir.mir: SessionAuthorizer::connection_is_allowed - this= SessionAuthorizer(0xffff981327d0) pid= 6814
[2020-07-15:10:58:27.215] ApplicationManager REJECTED connection from app with pid 6814 as it was not launched by upstart, and no desktop_file_hint is specified
[2020-07-15:10:58:27.764] qtmir.mir: SessionAuthorizer::connection_is_allowed - this= SessionAuthorizer(0xffff981327d0) pid= 6867
[2020-07-15:10:58:28.287] CursorImageProvider: "" not found (nor its fallbacks, if any). Going for "left_ptr" as a last resort.
[2020-07-15:10:58:28.315] QObject::connect: No such signal QDBusAbstractInterface::StartUrl(const QString &, const QString &)
[2020-07-15:10:58:28.316] QObject::connect: No such signal QDBusAbstractInterface::ShowHome(const QString &)

** (process:6738): CRITICAL **: Error parsing manifest for package 'mycontainer': mycontainer does not exist in any database for user phablet
[2020-07-15:10:58:29.456] qml: updateLightState: onDisplayStatusChanged, indicatorState: INDICATOR_OFF, supportsMultiColorLed: true, hasMessages: false, icon: battery-full-charging-symbolic, displayStatus: 1, deviceState: , batteryLevel: 0
[2020-07-15:10:58:29.488] qml: updateLightState: onBatteryIconNameChanged, indicatorState: INDICATOR_OFF, supportsMultiColorLed: true, hasMessages: false, icon: battery-full-charging-symbolic, displayStatus: 1, deviceState: , batteryLevel: 0
[2020-07-15:10:58:29.504] file:///usr/share/unity8//Panel/Indicators/IndicatorsLight.qml:207: Error: Cannot assign [undefined] to double
[2020-07-15:10:58:29.525] file:///usr/share/unity8//Panel/Indicators/IndicatorsLight.qml:211: Error: Cannot assign [undefined] to QString

** (process:6738): CRITICAL **: Error parsing manifest for package 'mycontainer': mycontainer does not exist in any database for user phablet
[2020-07-15:10:58:29.958] QQmlExpression: Expression file:///usr/share/unity8//DeviceConfiguration.qml:62:46 depends on non-NOTIFYable properties:
[2020-07-15:10:58:29.959]     DeviceConfig::supportsMultiColorLed
[2020-07-15:10:58:29.968] qtmir.mir.keymap: SET KEYMAP "us"
[2020-07-15:10:58:30.057] ubuntu-app-launch threw an exception getting app info for appId: "mycontainer_notification-daemon" : Application is not meant to be displayed
[2020-07-15:10:58:30.058] Failed to get app info for app "mycontainer_notification-daemon"

** (process:6738): CRITICAL **: Error parsing manifest for package 'mycontainer': mycontainer does not exist in any database for user phablet
[2020-07-15:10:58:30.349] ubuntu-app-launch threw an exception getting app info for appId: "mycontainer_org.gnome.gedit" : Application is not meant to be displayed
[2020-07-15:10:58:30.350] Failed to get app info for app "mycontainer_org.gnome.gedit"

** (process:6738): CRITICAL **: Error parsing manifest for package 'mycontainer': mycontainer does not exist in any database for user phablet
[2020-07-15:10:58:30.446] file:///usr/share/unity8//Components/ImageResolver.qml:41:19: QML QQuickImage: Cannot open: file:///custom/graphics/homebutton.svg
[2020-07-15:10:58:30.514] ubuntu-app-launch threw an exception getting app info for appId: "mycontainer_python3.5" : Application is not meant to be displayed
[2020-07-15:10:58:30.514] Failed to get app info for app "mycontainer_python3.5"

** (process:6738): CRITICAL **: Error parsing manifest for package 'mycontainer': mycontainer does not exist in any database for user phablet
Attempted to unregister path (path[0] = null path[1] = null) which isn't registered
[2020-07-15:10:58:31.767] toplevelwindowmodel: setRootFocus(false), surfaceManagerBusy is false
[2020-07-15:10:58:31.769] toplevelwindowmodel: setFocusedWindow(Window[0x1896eb80, id=0, null])
[2020-07-15:10:58:32.062] file:///usr/share/unity8//Components/ImageResolver.qml:41:19: QML QQuickImage: Cannot open: file:///usr/share/unity8/graphics/phone_background.jpg
[2020-07-15:10:58:32.245] qml: Calculating new usage mode. Pointer devices: 0 current mode: Staged old device count 0 root width: 40 height: 71
[2020-07-15:10:58:32.270] file:///usr/share/unity8//Panel/Indicators/IndicatorsLight.qml:216: ReferenceError: Lights is not defined
[2020-07-15:10:58:32.293] file:///usr/share/unity8//OrientedShell.qml:281:32: Unable to assign [undefined] to bool
[2020-07-15:10:58:32.294] qtmir.screens: Screen::setWindow - new geometry for shell surface ShellView(0x188d76f0) QRect(0,0 720x1440)
[2020-07-15:10:58:32.295] qtmir.screens: ScreenWindow 0x189a7940 with window ID 1 backed by Screen(0xffff98133960) with ID 0
[2020-07-15:10:58:32.295] qtmir.screens: QWindow ShellView(0x188d76f0) with geom QRect(0,0 720x1440) is backed by a Screen(0xffff98133960) with geometry QRect(0,0 720x1440)
[2020-07-15:10:58:32.305] qml: Calculating new usage mode. Pointer devices: 0 current mode: Staged old device count 0 root width: 51.42857142857143 height: 102.85714285714286
[2020-07-15:10:58:32.329] qtmir.screens: ScreensModel::onCompositorStarting
[2020-07-15:10:58:32.430] qtmir.sensor: OrientationSensor::readingChanged
[2020-07-15:10:58:32.430] qtmir.sensor: Screen::onOrientationReadingChanged
[2020-07-15:10:58:32.468] qtmir.screens: ScreensModel::update
[2020-07-15:10:58:32.468] qtmir.screens: Can reuse Screen with id 0
[2020-07-15:10:58:32.469] qtmir.screens: Screen::setMirDisplayBuffer Screen(0xffff98133960) 0xffff980a0788 0xffff980a0780
[2020-07-15:10:58:32.469] qtmir.screens: =======================================
[2020-07-15:10:58:32.469] qtmir.screens: Screen(0xffff98133960) - id: 0 geometry: QRect(0,0 720x1440) window: 0x189a7940 type: "Unknown" scale: 1.75
[2020-07-15:10:58:32.469] qtmir.screens: =======================================
[2020-07-15:10:58:32.469] qtmir.screens: ScreenWindow::setExposed 0x189a7940 true 0xffff98133970
[2020-07-15:10:58:32.476] qtmir.sessions: TaskController::onSessionStarting - sessionName=
[2020-07-15:10:58:34.957] Input device added: "gpio-keys" "/dev/input/event2" QFlags<QInputDevice::InputType>(Button)
[2020-07-15:10:58:34.967] Input device added: "1c21800.lradc" "/dev/input/event3" QFlags<QInputDevice::InputType>(Button)
[2020-07-15:10:58:34.972] Input device added: "Goodix Capacitive TouchScreen" "/dev/input/event1" QFlags<QInputDevice::InputType>(Button|TouchScreen)
[2020-07-15:10:58:34.975] Input device added: "axp20x-pek" "/dev/input/event0" QFlags<QInputDevice::InputType>(Button)
[2020-07-15:10:58:35.136] APP_ID isn't set, the handler ignored
[2020-07-15:10:58:35.975] [PERFORMANCE]: Last frame took 431 ms to render.
[2020-07-15:10:58:35.989] Unknown orientation.
[2020-07-15:10:58:36.074] qml: EdgeBarrierSettings: min=2gu(28px), max=120gu(1680px), sensitivity=0.5, threshold=61gu(854px)
[2020-07-15:10:58:36.140] [PERFORMANCE]: Last frame took 53 ms to render.
[2020-07-15:10:58:36.288] qml: updateLightState: onBatteryLevelChanged, indicatorState: INDICATOR_OFF, supportsMultiColorLed: true, hasMessages: false, icon: battery-full-charging-symbolic, displayStatus: 1, deviceState: , batteryLevel: 95
[2020-07-15:10:58:36.289] qml: updateLightState: onDeviceStateChanged, indicatorState: INDICATOR_OFF, supportsMultiColorLed: true, hasMessages: false, icon: battery-full-charging-symbolic, displayStatus: 1, deviceState: charging, batteryLevel: 95
[2020-07-15:10:58:37.508] [PERFORMANCE]: Last frame took 64 ms to render.
[2020-07-15:10:59:15.237] qml: updateLightState: onBatteryLevelChanged, indicatorState: INDICATOR_OFF, supportsMultiColorLed: true, hasMessages: false, icon: battery-full-charging-symbolic, displayStatus: 1, deviceState: charging, batteryLevel: 96
[2020-07-15:11:01:15.300] qml: updateLightState: onBatteryLevelChanged, indicatorState: INDICATOR_OFF, supportsMultiColorLed: true, hasMessages: false, icon: battery-full-charging-symbolic, displayStatus: 1, deviceState: charging, batteryLevel: 97
[2020-07-15:11:04:29.762] [PERFORMANCE]: Last frame took 45 ms to render.
[2020-07-15:11:04:34.709] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::Application()
[2020-07-15:11:04:34.710] toplevelwindowmodel: prependPlaceholder(com.ubuntu.terminal_terminal)
[2020-07-15:11:04:34.790] qtmir.mir: SessionAuthorizer::connection_is_allowed - this= SessionAuthorizer(0xffff981327d0) pid= 11695
[2020-07-15:11:04:34.965] file:///usr/share/unity8//Stage/Stage.qml:1877:17: QML WindowInfoItem: Binding loop detected for property "maxWidth"
[2020-07-15:11:04:35.017] toplevelwindowmodel: setFocusedWindow(Window[0x1a274750, id=1, null])
[2020-07-15:11:04:35.018] toplevelwindowmodel: prependSurfaceHelper after (index=0,appId=com.ubuntu.terminal_terminal,surface=0x0,id=1)
[2020-07-15:11:04:35.019] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::setRequestedState(requestedState=suspended)
[2020-07-15:11:04:35.057] qtmir.sessions: TaskController::onSessionStarting - sessionName=
[2020-07-15:11:04:35.058] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::addSession(session=qtmir::Session(0x1a7506e0))
[2020-07-15:11:04:35.346] toplevelwindowmodel: setRootFocus(true), surfaceManagerBusy is false
[2020-07-15:11:04:35.347] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::setRequestedState(requestedState=running)
[2020-07-15:11:04:36.571] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::MirSurface(type=freestyle,state=restored,size=(640,480),parentSurface=QObject(0x0))
[2020-07-15:11:04:36.572] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::activate()
[2020-07-15 11:04:36.574409] < -warning- > mirserver: WaylandInputDispatcher::input_consumed() got non-input event type 11
[2020-07-15:11:04:36.581] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::registerView(440057776) after=1
[2020-07-15:11:04:36.583] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::setKeymap("us")
[2020-07-15:11:04:36.584] toplevelwindowmodel: prependSurface appId=com.ubuntu.terminal_terminal surface=qtmir::MirSurface(0x1a31c340), filling out placeholder. after: (index=0,appId=com.ubuntu.terminal_terminal,surface=0x1a31c340,id=1)
[2020-07-15:11:04:36.584] toplevelwindowmodel: setFocusedWindow(0x0)
[2020-07-15:11:04:36.585] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::setFocused(true)
[2020-07-15:11:04:36.588] toplevelwindowmodel: setFocusedWindow(Window[0x1a274750, id=1, MirSurface[0x1a31c340,"ubuntu-terminal-app"]])
[2020-07-15:11:04:36.690] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::requestState(restored)
[2020-07-15:11:04:37.469] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::setReady()
[2020-07-15:11:04:37.470] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::setInternalState(state=Running)
[2020-07-15:11:04:37.476] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::updateExposure(true)
[2020-07-15:11:04:38.954] qtmir.surfaces: MirSurface[0x1a3155a0,""]::MirSurface(type=input Method,state=restored,size=(640,480),parentSurface=QObject(0x0))
[2020-07-15:11:04:38.955] qtmir.surfaces: MirSurface[0x1a3155a0,""]::registerView(413341760) after=1
[2020-07-15:11:04:39.242] qtmir.surfaces: MirSurface[0x1a3155a0,""]::setReady()
[2020-07-15:11:04:39.243] qtmir.surfaces: MirSurface[0x1a3155a0,""]::updateExposure(true)
[2020-07-15:11:05:03.192] [PERFORMANCE]: Last frame took 148 ms to render.
[2020-07-15:11:05:03.297] [PERFORMANCE]: Last frame took 37 ms to render.
[2020-07-15:11:05:04.709] qtmir.applications: Application["com.ubuntu.filemanager_filemanager"]::Application()
[2020-07-15:11:05:04.710] toplevelwindowmodel: prependPlaceholder(com.ubuntu.filemanager_filemanager)
[2020-07-15:11:05:04.864] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::setRequestedState(requestedState=suspended)
[2020-07-15:11:05:04.864] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::suspend()
[2020-07-15:11:05:04.864] qtmir.applications: Application["com.ubuntu.terminal_terminal"]::setInternalState(state=SuspendingWaitSession)
[2020-07-15:11:05:04.865] toplevelwindowmodel: setFocusedWindow(Window[0x18fa2fc0, id=3, null])
[2020-07-15:11:05:04.867] toplevelwindowmodel: prependSurfaceHelper after (index=0,appId=com.ubuntu.filemanager_filemanager,surface=0x0,id=3),(index=1,appId=com.ubuntu.terminal_terminal,surface=0x1a31c340,id=1)
[2020-07-15:11:05:04.871] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::setFocused(false)
[2020-07-15:11:05:05.082] qtmir.surfaces: MirSurface[0x1a31c340,"com.ubuntu.terminal_terminal"]::updateExposure(false)
[2020-07-15:11:05:05.166] qtmir.surfaces: MirSurface[0x1a3155a0,""]::setLive(false)
[2020-07-15:11:05:05.167] qtmir.surfaces: MirSurface[0x1a3155a0,""]::unregisterView(413341760) after=0 live=false
[2020-07-15:11:05:05.168] qtmir.surfaces: MirSurface[0x1a3155a0,""]::updateExposure(false)
[2020-07-15:11:05:05.169] qtmir.surfaces: MirSurface[0x1a3155a0,""]::~MirSurface() viewCount=0
[2020-07-15:11:05:05.341] qtmir.mir: SessionAuthorizer::connection_is_allowed - this= SessionAuthorizer(0xffff981327d0) pid= 12109
[2020-07-15:11:05:05.364] qtmir.sessions: TaskController::onSessionStarting - sessionName=
[2020-07-15:11:05:05.364] qtmir.applications: Application["com.ubuntu.filemanager_filemanager"]::addSession(session=qtmir::Session(0x19db4100))
[2020-07-15:11:05:05.369] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::MirSurface(type=freestyle,state=restored,size=(16,16),parentSurface=QObject(0x0))
[2020-07-15:11:05:05.370] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::activate()
[2020-07-15 11:05:05.370912] < -warning- > mirserver: WaylandInputDispatcher::input_consumed() got non-input event type 11
[2020-07-15:11:05:05.374] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::registerView(420469040) after=1
[2020-07-15:11:05:05.382] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::setKeymap("us")
[2020-07-15:11:05:05.383] toplevelwindowmodel: prependSurface appId=com.ubuntu.filemanager_filemanager surface=qtmir::MirSurface(0x1a3155a0), filling out placeholder. after: (index=0,appId=com.ubuntu.filemanager_filemanager,surface=0x1a3155a0,id=3),(index=1,appId=com.ubuntu.terminal_terminal,surface=0x1a31c340,id=1)
[2020-07-15:11:05:05.383] toplevelwindowmodel: setFocusedWindow(0x0)
[2020-07-15:11:05:05.383] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::setReady()
[2020-07-15:11:05:05.384] qtmir.applications: Application["com.ubuntu.filemanager_filemanager"]::setInternalState(state=Running)
[2020-07-15:11:05:05.388] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::setFocused(true)
[2020-07-15:11:05:05.391] toplevelwindowmodel: setFocusedWindow(Window[0x18fa2fc0, id=3, MirSurface[0x1a3155a0,""]])
[2020-07-15:11:05:05.525] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::requestState(restored)
[2020-07-15:11:05:05.528] qtmir.surfaces: MirSurface[0x1a3155a0,"com.ubuntu.filemanager_filemanager"]::updateExposure(true)
terminate called after throwing an instance of 'std::logic_error'
  what():  Buffer does not support GL rendering
()
```
