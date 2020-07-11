# pinephone-mir
Experiments with Mir display server on PinePhone with UbuntuTouch

## How to run `strace` on apps

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

Or look at /home/phablet/.cache/upstart/application-click-com.ubuntu.filemanager_filemanager_0.7.5.log
