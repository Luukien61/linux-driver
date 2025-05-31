```shell
sudo cp device_model/bex.c /var/lib/docker/volumes/SO2_DOCKER_VOLUME/_data/tools/labs/skels/device_model/bex.c
sudo cp device_model/bex_misc.c /var/lib/docker/volumes/SO2_DOCKER_VOLUME/_data/tools/labs/skels/device_model/
```

**To verify that the `bex` bus is exist:**
```shell
ls /sys/bus
ls /sys/bus/bex/devices
# Check their contents
cat /sys/bus/bex/devices/root/type
# Should show "none"
cat /sys/bus/bex/devices/root/version
# Should show "1"

echo "name type 1" > /sys/bus/bex/add
echo "name" > /sys/bus/bex/del

ls /sys/bus/bex/devices/

ls /sys/bus/bex/drivers/

echo "device1 misc 2" > /sys/bus/bex/add
echo "device1" > /sys/bus/bex/del

# to get major and minor number
cat /sys/bus/bex/devices/device1/misc/bex-misc-1/dev

mknod /dev/bex-device1 c <major> <minor>

echo "Hello from userspace" > /dev/bex-device1
echo "Hello from userspace" > /dev/bex-misc-1

cat /dev/bex-device1


udevadm monitor --kernel --property --subsystem-match=bex > device.log
```