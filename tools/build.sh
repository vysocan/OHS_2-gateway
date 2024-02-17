#!/bin/sh

set -ex

export PATH=/tmp/xpack-arm-none-eabi-gcc-9.2.1-1.1/bin:$PATH

make -j
