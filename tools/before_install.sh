#!/bin/sh

set -ex

cd /tmp

wget https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v11.3.1-1.1/xpack-arm-none-eabi-gcc-11.3.1-1.1-linux-x64.tar.gz
tar xzf xpack-arm-none-eabi-gcc-11.3.1-1.1-linux-x64.tar.gz
export PATH=/tmp/pack-arm-none-eabi-gcc-11.3.1-1.1/bin:$PATH
arm-none-eabi-gcc --version

cd -
cd ../..

git clone https://github.com/vysocan/ChibiOS_21.11.x.git chibios_stable-21.11.x