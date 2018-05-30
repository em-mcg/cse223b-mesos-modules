#!/bin/bash

MESOS_SRC="/home/erin/project-223b/mesos/src"

# make sure debug flags are enabled
CFLAGS="-g"
# LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:"/home/erin/project-223b/mesos/3rdparty/libprocess/build"


rm -rf build
mkdir build && cd build

../configure --with-mesos=/usr/local/lib --libdir=/home/erin/project-223b/mesos/build/3rdparty

cd build/; make; cd ..
