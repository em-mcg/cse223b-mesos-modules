#!/bin/bash

MESOS_SRC="/home/erin/project-223b/mesos/src"
CFLAGS="-I ${MESOS_SRC}"

rm -rf build
mkdir build && cd build

../configure --with-mesos=/usr/local/lib --with-mesos-root=${MESOS_SRC}

cd build/; make; cd ..