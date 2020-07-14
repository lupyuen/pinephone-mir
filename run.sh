#!/bin/bash

# Log Wayland messages
export WAYLAND_DEBUG=1

# Run sdl app
# export SDL_VIDEODRIVER=wayland
# export SDL_DEBUG=1
# ./sdl

# Run shm app
# ./shm

# Run egl2 app
# ./egl2

# Run File Manager
# filemanager

# Run egl2 app with strace
# ./strace \
#    -s 1024 \
#    ./egl2

# Run gtk app with strace
# ./strace \
#    -s 1024 \
#    ./gtk

# Run File Manager with strace
# ./strace \
#     -s 1024 \
#     filemanager

# Debug sdl app
export SDL_VIDEODRIVER=wayland
export SDL_DEBUG=1
gdb \
    -ex="r" \
    -ex="bt" \
    -ex="frame" \
    --args ./sdl

# Debug gtk app
# gdb \
#     -ex="r" \
#     -ex="show auto-solib-add" \
#     -ex="info sharedlibrary" \
#     -ex="info sources" \
#     -ex="bt" \
#     -ex="frame" \
#     -ex="info locals" \
#     -ex="info args" \
#     --args ./gtk