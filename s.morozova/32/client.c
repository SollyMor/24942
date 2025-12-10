#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/un.h>

static const char *socket_path = "/tmp/server_socket";

static ssize_t robust_write(int fd, const void *buf, size_t count) {
    const char *p = (const char *)buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)n;
        left -= (size_t)n;
    }
    return (ssize_t)count;
}

int main(int argc, char *argv[]) {
    // Get client number from command line argument, default to "1"
    const char *client_num = (argc > 1) ? argv[1] : "1";
    char message[32];
    snprintf(message, sizeof(message), "Client%s\n", client_num);
    size_t message_len = strlen(message);

    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }
    struct timespec start_time, current_time;

    clock_gettime(CLOCK_MONOTONIC, &start_time);

    while (1) {
        clock_gettime(CLOCK_MONOTONIC, &current_time);

        // Check if 2 seconds have elapsed
        long elapsed_ms = (current_time.tv_sec - start_time.tv_sec) * 1000L +
                         (current_time.tv_nsec - start_time.tv_nsec) / 1000000L;
        if (elapsed_ms >= 2000) {
            break;
        }

        ssize_t bytes_written = robust_write(fd, message, message_len);
        if (bytes_written < 0) {
            perror("write");
            close(fd);
            return 1;
        }
        if ((size_t)bytes_written != message_len) {
            fprintf(stderr, "Partial write: wrote %zd of %zu bytes\n", bytes_written, message_len);
        }

        struct timespec sleep_time = {0, 10000000}; // 10 ms between sends
        nanosleep(&sleep_time, NULL);
    }

    close(fd);
    return 0;
}