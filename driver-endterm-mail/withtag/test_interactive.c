// test_tagged_interactive.c - Interactive test program with tag support
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define MAX_TAG_SIZE 32
#define MAX_MESSAGE_SIZE 1024
#define DEFAULT_TAG "public"

// IOCTL commands (phải khớp với driver)
#define MAILBOX_IOC_MAGIC 'm'
#define MAILBOX_IOC_SET_TAG _IOW(MAILBOX_IOC_MAGIC, 1, char*)
#define MAILBOX_IOC_GET_TAG _IOR(MAILBOX_IOC_MAGIC, 2, char*)
#define MAILBOX_IOC_CLEAR_TAG _IO(MAILBOX_IOC_MAGIC, 3)

void print_help() {
    printf("\nAvailable commands:\n");
    printf("  help       - Show this help\n");
    printf("  settag     - Set tag for this device\n");
    printf("  gettag     - Get current tag\n");
    printf("  cleartag   - Reset to default tag ('%s')\n", DEFAULT_TAG);
    printf("  send       - Send a message (uses default tag '%s' if none set)\n", DEFAULT_TAG);
    printf("  recv       - Receive a message (uses default tag '%s' if none set)\n", DEFAULT_TAG);
    printf("  quit       - Exit program\n\n");
}

int set_tag(int fd) {
    char tag[MAX_TAG_SIZE];

    printf("Enter tag (max %d chars): ", MAX_TAG_SIZE - 1);
    if (!fgets(tag, sizeof(tag), stdin)) {
        return -1;
    }

    // Remove newline
    tag[strcspn(tag, "\n")] = 0;

    if (strlen(tag) == 0) {
        printf("Tag cannot be empty\n");
        return -1;
    }

    if (ioctl(fd, MAILBOX_IOC_SET_TAG, tag) < 0) {
        perror("Failed to set tag");
        return -1;
    }

    printf("Tag set to: '%s'\n", tag);
    return 0;
}

int get_tag(int fd) {
    char tag[MAX_TAG_SIZE];

    if (ioctl(fd, MAILBOX_IOC_GET_TAG, tag) < 0) {
        perror("Failed to get tag");
        return -1;
    }

    printf("Current tag: '%s'\n", tag);
    return 0;
}

int clear_tag(int fd) {
    if (ioctl(fd, MAILBOX_IOC_CLEAR_TAG) < 0) {
        perror("Failed to clear tag");
        return -1;
    }

    printf("Tag reset to default: '%s'\n", DEFAULT_TAG);
    return 0;
}

int send_message(int fd) {
    char message[MAX_MESSAGE_SIZE];

    printf("Enter message to send: ");
    if (!fgets(message, sizeof(message), stdin)) {
        return -1;
    }

    // Remove newline
    message[strcspn(message, "\n")] = 0;

    if (strlen(message) == 0) {
        printf("Message cannot be empty\n");
        return -1;
    }

    ssize_t bytes_written = write(fd, message, strlen(message));
    if (bytes_written < 0) {
        perror("Failed to send message");
        return -1;
    }

    printf("Sent %zd bytes\n", bytes_written);
    return 0;
}

int receive_message(int fd) {
    char buffer[MAX_MESSAGE_SIZE];
    char input[64];
    int bytes_to_read;

    printf("Enter number of bytes to receive (max %d): ", MAX_MESSAGE_SIZE - 1);
    if (!fgets(input, sizeof(input), stdin)) {
        return -1;
    }

    bytes_to_read = atoi(input);
    if (bytes_to_read <= 0 || bytes_to_read >= MAX_MESSAGE_SIZE) {
        printf("Invalid number of bytes\n");
        return -1;
    }

    printf("Waiting for message (may block)...\n");
    ssize_t bytes_read = read(fd, buffer, bytes_to_read);
    if (bytes_read < 0) {
        perror("Failed to receive message");
        return -1;
    }

    buffer[bytes_read] = '\0';
    printf("Received %zd bytes: '%s'\n", bytes_read, buffer);
    return 0;
}

int main(int argc, char *argv[]) {
    int fd;
    char *device = "/dev/mailbox0";
    char command[64];

    if (argc >= 2) {
        device = argv[1];
    }

    printf("Tagged Mailbox Test Program\n");
    printf("Opening device: %s\n", device);

    fd = open(device, O_RDWR);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }

    printf("Device opened successfully with default tag '%s'\n", DEFAULT_TAG);
    print_help();

    while (1) {
        printf("mailbox> ");
        fflush(stdout);
        
        if (!fgets(command, sizeof(command), stdin)) {
            break;
        }

        // Remove newline
        command[strcspn(command, "\n")] = 0;

        if (strlen(command) == 0) {
            continue;
        }

        if (strcmp(command, "help") == 0) {
            print_help();
        }
        else if (strcmp(command, "settag") == 0) {
            set_tag(fd);
        }
        else if (strcmp(command, "gettag") == 0) {
            get_tag(fd);
        }
        else if (strcmp(command, "cleartag") == 0) {
            clear_tag(fd);
        }
        else if (strcmp(command, "send") == 0) {
            send_message(fd);
        }
        else if (strcmp(command, "recv") == 0) {
            receive_message(fd);
        }
        else if (strcmp(command, "quit") == 0) {
            break;
        }
        else {
            printf("Unknown command: %s\n", command);
            printf("Type 'help' for available commands\n");
        }
    }

    close(fd);
    printf("Goodbye!\n");
    return 0;
}