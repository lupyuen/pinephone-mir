#!/bin/bash

# Debug gtk app
gdb \
    -ex="r" \
    -ex="show auto-solib-add" \
    -ex="info sharedlibrary" \
    -ex="info sources" \
    -ex="bt" \
    -ex="frame" \
    -ex="info locals" \
    -ex="info args" \
    --args ./gtk

# strace gtk app
# ./strace \
#    -s 1024 \
#    ./gtk

# strace File Manager
# ./strace \
#     -s 1024 \
#     filemanager
