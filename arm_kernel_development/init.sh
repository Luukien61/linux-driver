#!/bin/bash

# Cập nhật và cài đặt công cụ
sudo apt-get update -y
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf -y

# Biên dịch kernel
cd /linux
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make imx_v6_v7_defconfig
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make -j8

# Biên dịch rootfs
cd tools/labs/
ARCH=arm make core-image-minimal-qemuarm.ext4

# Clone và biên dịch QEMU
cd /linux
if [ ! -d qemu ]; then
    git clone https://gitlab.com/qemu-project/qemu.git -b v9.0.0
fi
cd qemu

# Cài đặt Python 3.12 và các công cụ cần thiết
sudo add-apt-repository ppa:deadsnakes/ppa -y
sudo apt update -y
sudo apt install python3.10 python3.10-venv meson ninja-build -y
python3.10 -m ensurepip --upgrade
python3.10 -m pip install --upgrade pip
python3.10 -m pip install tomli
sudo update-alternatives --install /usr/bin/python3 python3 /usr/bin/python3.10 1
sudo update-alternatives --config python3
# grep -rl '/usr/bin/python3.12' ~/.local/bin /usr/local/bin 2>/dev/null

# Cài đặt GLib 2.78
if [ ! -f glib-2.78.0.tar.xz ]; then
    wget https://download.gnome.org/sources/glib/2.78/glib-2.78.0.tar.xz
fi
if [ ! -d glib-2.78.0 ]; then
    tar -xf glib-2.78.0.tar.xz
fi
cd glib-2.78.0
pip3.10 install --user --upgrade meson
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
meson setup builddir
cd builddir
ninja
sudo ninja install

# Cập nhật biến môi trường
export PATH=$HOME/.local/bin:$PATH
export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH
source ~/.bashrc

# Biên dịch QEMU
cd /linux/qemu
./configure --target-list=arm-softmmu --disable-docs
make -j8
cd /linux
./qemu/build/qemu-system-arm -M mcimx6ul-evk -cpu cortex-a7 -m 512M \
  -kernel arch/arm/boot/zImage -nographic  -dtb arch/arm/boot/dts/imx6ul-14x14-evk.dtb \
  -append "root=/dev/mmcblk0 rw console=ttymxc0 loglevel=8 earlycon printk" -sd tools/labs/core-image-minimal-qemuarm.ext4