// test_interactive.c - Interactive test program
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

int main(int argc, char *argv[]) {
    int fd;
    char *device = "/dev/mailbox0";
    char buffer[1024];
    char input[1024];
    int mode = 0; // 0 = write, 1 = read

    if (argc >= 2) {
        device = argv[1];
    }
    if (argc >= 3) {
        mode = atoi(argv[2]);
    }

    printf("Opening device: %s in %s mode\n", device, mode ? "READ" : "WRITE");
    fd = open(device, mode ? O_RDONLY : O_WRONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    if (mode == 0) {  // Write mode
        while (1) {
            printf("Enter message (or 'quit' to exit): ");
            if (!fgets(input, sizeof(input), stdin)) {
                break;
            }

            // Remove newline
            input[strcspn(input, "\n")] = 0;

            if (strcmp(input, "quit") == 0) {
                break;
            }

            ssize_t bytes_written = write(fd, input, strlen(input));
            if (bytes_written < 0) {
                perror("Write failed");
            } else {
                printf("Wrote %zd bytes\n", bytes_written);
            }
        }
    } else {  // Read mode
        while (1) {
            printf("Enter number of bytes to read (or 0 to exit): ");
            if (!fgets(input, sizeof(input), stdin)) {
                break;
            }

            int bytes_to_read = atoi(input);
            if (bytes_to_read <= 0) {
                break;
            }

            if (bytes_to_read >= sizeof(buffer)) {
                printf("Too many bytes requested\n");
                continue;
            }

            printf("Reading %d bytes (may block)...\n", bytes_to_read);
            ssize_t bytes_read = read(fd, buffer, bytes_to_read);
            if (bytes_read < 0) {
                perror("Read failed");
            } else {
                buffer[bytes_read] = '\0';
                printf("Read %zd bytes: '%s'\n", bytes_read, buffer);
            }
        }
    }

    close(fd);
    return 0;
}