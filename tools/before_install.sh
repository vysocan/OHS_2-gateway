#!/bin/sh

set -ex

cd /tmp

wget https://github.com/xpack-dev-tools/arm-none-eabi-gcc-xpack/releases/download/v9.2.1-1.1/xpack-arm-none-eabi-gcc-9.2.1-1.1-linux-x64.tar.gz
tar xzf xpack-arm-none-eabi-gcc-9.2.1-1.1-linux-x64.tar.gz
export PATH=/tmp/xpack-arm-none-eabi-gcc-9.2.1-1.1/bin:$PATH
arm-none-eabi-gcc --version

cd -
cd ../..

git clone https://github.com/vysocan/OHS_2-chibios.git chibios_stable-20.3.x
