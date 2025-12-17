#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/poll.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define SOCKET_PATH "task31_socket"
#define MAX_CLIENTS 10          // ↑ увеличено, чтобы принять больше клиентов
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int messages_received;
    char buf[BUFFER_SIZE];
    size_t buf_len;
} client_state_t;

int main() {
    unlink(SOCKET_PATH);

    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr = {.sun_family = AF_UNIX};
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s (will run for 5 seconds)\n", SOCKET_PATH);

    struct pollfd fds[MAX_CLIENTS + 1];
    client_state_t clients[MAX_CLIENTS] = {{0}};

    int nfds = 1;
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].fd = -1;
        clients[i].messages_received = 0;
        clients[i].buf_len = 0;
        fds[i + 1].fd = -1;
        fds[i + 1].events = POLLIN;
    }

    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);

    double elapsed = 0.0;
    const double RUN_DURATION = 10.0; // 5 секунд
    int total_messages = 0;
    while (elapsed < RUN_DURATION) {
        // Вычисляем, сколько миллисекунд осталось
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        elapsed = (now.tv_sec - start_time.tv_sec) +
                  (now.tv_nsec - start_time.tv_nsec) / 1e9;

        int timeout_ms = (int)((RUN_DURATION - elapsed) * 1000);
        if (timeout_ms <= 0) break;

        int ready = poll(fds, nfds, timeout_ms);
        if (ready == -1) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }

        // Новое подключение
        if (fds[0].revents & POLLIN) {
            int client_fd = accept(server_fd, NULL, NULL);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }

            int slot = -1;
            for (int i = 0; i < MAX_CLIENTS; ++i) {
                if (clients[i].fd == -1) {
                    slot = i;
                    break;
                }
            }
            if (slot == -1) {
                close(client_fd);
            } else {
                clients[slot].fd = client_fd;
                clients[slot].messages_received = 0;
                clients[slot].buf_len = 0;
                fds[slot + 1].fd = client_fd;
                if (slot + 2 > nfds) nfds = slot + 2;
            }
        }

        // Обработка клиентов
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (clients[i].fd == -1) continue;

            int idx = i + 1;
            if (fds[idx].revents & (POLLIN | POLLHUP | POLLERR)) {
                char tmp_buf[BUFFER_SIZE];
                ssize_t n = read(clients[i].fd, tmp_buf, sizeof(tmp_buf));
                if (n <= 0) {
                    // Клиент отключился — обработать остаток буфера
                    if (clients[i].buf_len > 0) {
                        for (size_t j = 0; j < clients[i].buf_len; ++j) {
                            clients[i].buf[j] = toupper((unsigned char)clients[i].buf[j]);
                        }
                        write(STDOUT_FILENO, clients[i].buf, clients[i].buf_len);
                        clients[i].messages_received++;
                    }
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    fds[idx].fd = -1;
                    while (nfds > 1 && fds[nfds - 1].fd == -1) nfds--;
                } else {
                    // Добавить в буфер
                    if (clients[i].buf_len + n > BUFFER_SIZE - 1) {
                        clients[i].buf_len = 0; // сброс при переполнении
                    } else {
                        memcpy(clients[i].buf + clients[i].buf_len, tmp_buf, n);
                        clients[i].buf_len += n;
                    }

                    // Обработать все строки по \n
                    while (1) {
                        char *newline = memchr(clients[i].buf, '\n', clients[i].buf_len);
                        if (!newline) break;

                        size_t line_len = newline - clients[i].buf + 1;
                        for (size_t j = 0; j < line_len; ++j) {
                            clients[i].buf[j] = toupper((unsigned char)clients[i].buf[j]);
                        }
                        total_messages++;
                        write(STDOUT_FILENO, clients[i].buf, line_len);
                        clients[i].messages_received++;

                        memmove(clients[i].buf, newline + 1, clients[i].buf_len - line_len);
                        clients[i].buf_len -= line_len;
                    }
                }
            }
        }
    }

    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double duration = (end_time.tv_sec - start_time.tv_sec) +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }
    printf("\n--- Server finished after %.3f seconds ---\n", duration);
    printf("Total messages processed: %d\n", total_messages);
    close(server_fd);
    unlink(SOCKET_PATH);
    return 0;
}