set -e -x

gcc \
    -g \
    -o basic \
    basic.c \
    `pkg-config --libs --cflags mirclient`
