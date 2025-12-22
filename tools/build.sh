#!/bin/sh

set -ex

export PATH=/tmp/pack-arm-none-eabi-gcc-11.3.1-1.1/bin:$PATH

make -j
