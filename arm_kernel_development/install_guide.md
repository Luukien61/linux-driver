```shell
docker volume rm SO2_DOCKER_VOLUME
sudo apt-get update
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
cd /linux
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make imx_v6_v7_defconfig
ARCH=arm CROSS_COMPILE=arm-linux-gnueabihf- make -j8
cd tools/labs/
ARCH=arm make core-image-minimal-qemuarm.ext4
cd /linux
git clone https://gitlab.com/qemu-project/qemu.git
cd qemu
sudo add-apt-repository ppa:deadsnakes/ppa
sudo apt update
sudo apt install python3.10
sudo apt install python3.10-venv
python3.10 -m ensurepip --upgrade
python3.10 -m pip install --upgrade pip
python3.10 -m pip install tomli
sudo apt install meson ninja-build
wget https://download.gnome.org/sources/glib/2.78/glib-2.78.0.tar.xz
tar -xf glib-2.78.0.tar.xz
cd glib-2.78.0
pip3.10 install --user --upgrade meson
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
meson setup builddir
cd builddir
ninja
sudo ninja install
export PATH=$HOME/.local/bin:$PATH
export LD_LIBRARY_PATH=$HOME/.local/lib:$LD_LIBRARY_PATH
export PKG_CONFIG_PATH=$HOME/.local/lib/pkgconfig:$PKG_CONFIG_PATH
source ~/.bashrc
cd /linux/qemu
./configure --target-list=arm-softmmu --disable-docs
make -j8
cd /linux
./qemu/build/qemu-system-arm -M mcimx6ul-evk -cpu cortex-a7 -m 512M \
  -kernel arch/arm/boot/zImage -nographic  -dtb arch/arm/boot/dts/imx6ul-14x14-evk.dtb \
  -append "root=/dev/mmcblk0 rw console=ttymxc0 loglevel=8 earlycon printk" -sd tools/labs/core-image-minimal-qemuarm.ext4
```
```shell
root@luukien-X415EA:/linux/tools/labs# find / -name "imx6ul-14x14-evk.dts" 2>/dev/null
cat /linux/arch/arm/boot/dts/imx6ul.dtsi

```
### Host
```shell
sudo cp arm_kernel_development/Makefile /var/lib/docker/volumes/SO2_DOCKER_VOLUME_ARM/_data/tools/labs/qemu/Makefile
sudo find /var/lib/docker/volumes/SO2_DOCKER_VOLUME_ARM/_data/ -name "imx6ul.dtsi" -type f 2>/dev/null
docker commit <container_id_or_name> <new_image_name>:<tag>
```

```text
Creating temporary directory: /tmp/tmp.AQFka4RHQ9
Mounting image: core-image-minimal-qemux86.ext4 to /tmp/tmp.AQFka4RHQ9
Copying files to: /tmp/tmp.AQFka4RHQ9/home/root
Unmounting: /tmp/tmp.AQFka4RHQ9
Removing temporary directory: /tmp/tmp.AQFka4RHQ9



sudo mkdir /mnt/temp
sudo mount -o loop tools/labs/core-image-minimal-qemuarm.ext4 /mnt/temp
ls /mnt/temp/home/root  # Kiểm tra file có tồn tại không
sudo umount /mnt/temp
```