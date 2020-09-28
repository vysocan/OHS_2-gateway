#!/bin/sh

set -ex

export PATH=/tmp/xpack-arm-none-eabi-gcc-9.2.1-1.1/bin:$PATH

pwd
cd ..
pwd
cd OHS_2-gateway
pwd


make -j
