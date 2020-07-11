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

Change the `Exec` line to...

```
Exec=./run.sh
```

Save and exit `nano`

Create `run.sh` with `nano`, containing:

```bash
#!/bin/bash
./strace ./gtk
```

Copy `strace`, `gtk` and set ownership...

```bash
cp /usr/bin/strace .

cp /home/phablet/pinephone-mir/gtk .

chown clickpkg:clickpkg strace gtk run.sh
```

Tap on File Manager icon on PinePhone to start the `strace`

Check the `strace` log in Logviewer under `v0.7.5 filemanager`

Or look at:

`/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log`

Sample `strace` log for `gtk` app:

https://github.com/lupyuen/pinephone-mir/blob/master/logs/gtk-strace.log
