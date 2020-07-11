# pinephone-mir
Experiments with Wayland and Mir display server on PinePhone with Ubuntu Touch

## How to run `strace` on the `gtk` app

Assume that we have built the `gtk` app by running [`gtk.sh`](gtk.sh)

```bash
sudo bash
cd /usr/share/click/preinstalled/.click/users/@all/com.ubuntu.filemanager
nano com.ubuntu.filemanager.desktop 
```

Change the `Exec` line to:

```
Exec=./run.sh
```

Create `run.sh`:

```bash
#!/bin/bash
./strace ./gtk
```

Copy `strace`, `gtk` and set permissions:

```bash
cp /usr/bin/strace .
cp /home/phablet/pinephone-mir/gtk .
chown clickpkg:clickpkg strace gtk run.sh
```

Tap on File Manager icon on PinePhone.

Check Logview for the `strace` log.

Or look at:

`/home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log`

Sample `strace` log for `gtk` app:

https://github.com/lupyuen/pinephone-mir/blob/master/logs/gtk-strace.log
