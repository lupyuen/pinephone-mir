set -e -x
gcc -o basic `pkg-config --libs --cflags mirclient` basic.c
