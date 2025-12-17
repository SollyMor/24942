#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define DEFAULT_PREFIX "msg"
#define SEND_DURATION_SEC 0.002

static double elapsed_seconds(const struct timeval *start, const struct timeval *current) {
    double seconds = (double)(current->tv_sec - start->tv_sec);
    double useconds = (double)(current->tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

static int send_message_nonblocking(int fd, const char *message) {
    size_t total_written = 0;
    size_t len = strlen(message);

    while (total_written < len) {
        ssize_t written = write(fd, message + total_written, len - total_written);
        if (written == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(1000);
                continue;
            } else if (errno == EPIPE) {
                // fprintf(stderr, "Server closed connection (broken pipe).\n");
                return -1;
            }
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
        total_written += (size_t)written;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    int flags;
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    char message1[BUFFER_SIZE];
    char message2[BUFFER_SIZE];
    struct timeval start_time;
    struct timeval current_time;
    int toggle = 0;
    
    signal(SIGPIPE, SIG_IGN);

    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Делаем сокет неблокирующим для асинхронного I/O
    flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Устанавливаем соединение с сервером
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("connect");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    }
    
    // Ждем завершения подключения
    fd_set write_fds;
    struct timeval timeout;
    FD_ZERO(&write_fds);
    FD_SET(client_fd, &write_fds);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (select(client_fd + 1, NULL, &write_fds, NULL, &timeout) <= 0) {
        perror("select on connect");
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
        if (send_message_nonblocking(client_fd, message) == -1) {
            // break;
        }
        toggle = !toggle;
        // usleep(500000);
        gettimeofday(&current_time, NULL);
    }
    
    // Закрываем соединение (разрыв соединения)
    close(client_fd);
    
    return EXIT_SUCCESS;
}

