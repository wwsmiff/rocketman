#!/bin/bash

set -xe

CC="g++"
CFLAGS="-Wall -std=c++20 -O2 -o"
LFLAGS="-lsfml-window -lsfml-system -lsfml-graphics -lm"
EXE="rocketman"
SRC="main.cpp random.cpp"
INCLUDEDIR=""

$CC $CFLAGS $SRC -o $EXE $LFLAGS
