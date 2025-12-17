#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/task31_socket"
#define BUFFER_SIZE 1024
#define DEFAULT_PREFIX "msg"
#define SEND_DURATION_SEC 0.002

static double elapsed_seconds(const struct timeval *start, const struct timeval *current) {
    double seconds = (double)(current->tv_sec - start->tv_sec);
    double useconds = (double)(current->tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

static void send_with_retry(int fd, const char *message) {
    size_t to_write = strlen(message);
    size_t written_total = 0;

    while (written_total < to_write) {
        ssize_t written = write(fd, message + written_total, to_write - written_total);
        if (written == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
        written_total += (size_t)written;
    }
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    char message1[BUFFER_SIZE];
    char message2[BUFFER_SIZE];
    struct timeval start_time;
    struct timeval current_time;
    int toggle = 0;
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--prefix=", 9) == 0 && strlen(argv[i] + 9) > 0) {
            strncpy(prefix, argv[i] + 9, sizeof(prefix) - 1);
            prefix[sizeof(prefix) - 1] = '\0';
        }
    }

    snprintf(message1, sizeof(message1), "%s1\n", prefix);
    snprintf(message2, sizeof(message2), "%s2\n", prefix);

    gettimeofday(&start_time, NULL);
    current_time = start_time;

    while (elapsed_seconds(&start_time, &current_time) < SEND_DURATION_SEC) {
        const char *message = toggle ? message2 : message1;
        send_with_retry(client_fd, message);
        toggle = !toggle;
        //usleep(10); // small pause between messages
        gettimeofday(&current_time, NULL);
    }
    
    close(client_fd);
    
    return EXIT_SUCCESS;
}

