// test_reader.c - Program để test việc đọc từ mailbox
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    int fd;
    char *device = "/dev/mailbox1";  // Default device
    int bytes_to_read = 10;          // Default bytes to read
    char buffer[1024];

    if (argc >= 2) {
        device = argv[1];
    }
    if (argc >= 3) {
        bytes_to_read = atoi(argv[2]);
        if (bytes_to_read <= 0 || bytes_to_read >= sizeof(buffer)) {
            fprintf(stderr, "Invalid number of bytes to read\n");
            return 1;
        }
    }

    printf("Opening device: %s\n", device);
    fd = open(device, O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("Attempting to read %d bytes (may block if no data)...\n", bytes_to_read);
    ssize_t bytes_read = read(fd, buffer, bytes_to_read);
    if (bytes_read < 0) {
        perror("Failed to read from device");
        close(fd);
        return 1;
    }

    buffer[bytes_read] = '\0';  // Null terminate for printing
    printf("Successfully read %zd bytes: '%s'\n", bytes_read, buffer);

    close(fd);
    return 0;
}