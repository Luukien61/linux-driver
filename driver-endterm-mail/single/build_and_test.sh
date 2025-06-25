#!/bin/bash

# build_and_test.sh - Script để build và test mailbox driver

echo "=== Mailbox Driver Build and Test Script ==="

# Function to check if command succeeded
check_result() {
    if [ $? -ne 0 ]; then
        echo "Error: $1 failed"
        exit 1
    fi
}


# Build user space programs
echo "2. Building user space programs..."
gcc -o test_writer test_writer.c
check_result "test_writer build"

gcc -o test_reader test_reader.c
check_result "test_reader build"

gcc -o test_interactive test_interactive.c
check_result "test_interactive build"

echo "3. Installing kernel module..."
sudo rmmod mailbox 2>/dev/null  # Remove if already loaded
sudo insmod mailbox_driver.ko
check_result "Module installation"

# Set permissions
sudo chmod 666 /dev/mailbox0
sudo chmod 666 /dev/mailbox1
check_result "Setting permissions"

echo "4. Showing device information..."
ls -la /dev/mailbox*
lsmod | grep mailbox
dmesg | tail -10

echo ""
echo "=== Build completed successfully! ==="
echo ""
echo "Usage examples:"
echo "  # Test single write and read:"
echo "  ./test_writer /dev/mailbox0 'ABC'"
echo "  ./test_reader /dev/mailbox1 2"
echo ""
echo "  # Interactive mode:"
echo "  ./test_interactive /dev/mailbox0 0  # Writer mode"
echo "  ./test_interactive /dev/mailbox1 1  # Reader mode"
echo ""
echo "  # Test scenario from problem description:"
echo "  # Terminal 1: ./test_interactive /dev/mailbox0 0"
echo "  # Terminal 2: ./test_interactive /dev/mailbox0 0"
echo "  # Terminal 3: ./test_interactive /dev/mailbox1 1"
echo "  # Then type 'ABC' in terminal 1, 'XY' in terminal 2"
echo "  # Read 2 bytes in terminal 3 (should get 'AB')"
echo "  # Read 4 bytes in terminal 3 (should get 'CXY' + block for 1 more byte)"
echo ""
echo "To remove module: sudo rmmod mailbox"