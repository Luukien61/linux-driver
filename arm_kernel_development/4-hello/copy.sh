#!/bin/bash

YOCTO_IMAGE=$1
TEMPDIR=`mktemp -u`

# $1 - target directory
# $2 - prefix for cp command (e.g. sudo)
do_copy()
{
  find skels -type f \( -name "*.ko" -or -executable \) | while read -r file; do
    echo "Copying file: $file to $1"
    $2 cp --parents "$file" -t "$1"
  done
  find skels -type d \( -name checker \) | while read -r dir; do
    echo "Copying directory: $dir to $1"
    $2 cp -r --parents "$dir" -t "$1"
  done
}

if  [ -e qemu.mon ]; then
  ip=`tail -n1 /tmp/dnsmasq-lkt-tap0.leases | cut -f3 -d ' '`
  if [ -z "$ip" ]; then
    echo "qemu is running and no IP address found"
    exit 1
  fi
  mkdir $TEMPDIR
  do_copy $TEMPDIR
  scp -q -r -O -o StrictHostKeyChecking=no $TEMPDIR/* root@$ip:.
  rm -rf $TEMPDIR
else
  echo "Creating temporary directory: $TEMPDIR"
  mkdir $TEMPDIR
  echo "Mounting image: $YOCTO_IMAGE to $TEMPDIR"
  sudo mount -t ext4 -o loop $YOCTO_IMAGE $TEMPDIR
  echo "Copying files to: $TEMPDIR/home/root"
  do_copy $TEMPDIR/home/root sudo
  echo "Unmounting: $TEMPDIR"
  sudo umount $TEMPDIR
  echo "Removing temporary directory: $TEMPDIR"
  rmdir $TEMPDIR
fi
