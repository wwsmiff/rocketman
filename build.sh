#!/bin/bash

set -xe

CC="g++"
CFLAGS="-Wall -std=c++20 -O2"
LFLAGS="-lsfml-window -lsfml-system -lsfml-graphics -lm"
EXE="rocketman"
SRC="main.cpp random.cpp"
INCLUDEDIR=""

build() {
  $CC $CFLAGS $SRC -o $EXE $LFLAGS
}

clean() {
  rm $EXE
}

$@
