#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

#define SOCKET_PATH "task32_socket"
#define MAX_CLIENTS 50          // ↑ увеличено для 10 сек
#define BUFFER_SIZE 2048
#define MAX_EVENTS 10

typedef struct {
    int fd;
    int messages_received;
    char buf[BUFFER_SIZE];
    size_t buf_len;
} client_state_t;

client_state_t clients[MAX_CLIENTS];

void init_clients() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        clients[i].fd = -1;
        clients[i].messages_received = 0;
        clients[i].buf_len = 0;
    }
}

int find_free_slot() {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd == -1) return i;
    }
    return -1;
}

int find_client_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd == fd) return i;
    }
    return -1;
}

int main() {
    unlink(SOCKET_PATH);
    int total_messages = 0;

    int server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un server_addr = {.sun_family = AF_UNIX};
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on %s (will run for 10 seconds)\n", SOCKET_PATH);

    init_clients();

    int epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev = {0};
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) == -1) {
        perror("epoll_ctl: server_socket");
        close(server_socket);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    // ЗАПУСКАЕМ ТАЙМЕР В НАЧАЛЕ
    struct timespec start_time;
    clock_gettime(CLOCK_MONOTONIC, &start_time);
    const double RUN_DURATION = 10.0; // 10 секунд

    while (1) {
        struct timespec now;
        clock_gettime(CLOCK_MONOTONIC, &now);
        double elapsed = (now.tv_sec - start_time.tv_sec) +
                        (now.tv_nsec - start_time.tv_nsec) / 1e9;

        if (elapsed >= RUN_DURATION) break;

        int timeout_ms = (int)((RUN_DURATION - elapsed) * 1000);
        if (timeout_ms <= 0) break;

        struct epoll_event events[MAX_EVENTS];
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, timeout_ms);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            int fd = events[i].data.fd;

            if (fd == server_socket) {
                // Новое подключение
                int client_fd = accept4(server_socket, NULL, NULL, SOCK_NONBLOCK);
                if (client_fd == -1) {
                    if (errno != EAGAIN && errno != EWOULDBLOCK)
                        perror("accept4");
                    continue;
                }

                int slot = find_free_slot();
                if (slot == -1) {
                    close(client_fd);
                } else {
                    clients[slot].fd = client_fd;
                    clients[slot].messages_received = 0;
                    clients[slot].buf_len = 0;

                    ev.events = EPOLLIN | EPOLLRDHUP;
                    ev.data.fd = client_fd;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                        perror("epoll_ctl: add client");
                        close(client_fd);
                        clients[slot].fd = -1;
                    }
                }
            } else {
                // Данные от клиента
                int client_idx = find_client_by_fd(fd);
                if (client_idx == -1) {
                    close(fd);
                    continue;
                }

                char tmp_buf[BUFFER_SIZE];
                ssize_t n = read(fd, tmp_buf, sizeof(tmp_buf));
                if (n <= 0) {
                    // Клиент отключился
                    if (clients[client_idx].buf_len > 0) {
                        for (size_t j = 0; j < clients[client_idx].buf_len; ++j) {
                            clients[client_idx].buf[j] = toupper((unsigned char)clients[client_idx].buf[j]);
                        }
                        write(STDOUT_FILENO, clients[client_idx].buf, clients[client_idx].buf_len);
                        clients[client_idx].messages_received++;
                    }

                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    clients[client_idx].fd = -1;
                } else {
                    // Добавить в буфер
                    if (clients[client_idx].buf_len + n > BUFFER_SIZE - 1) {
                        clients[client_idx].buf_len = 0;
                    } else {
                        memcpy(clients[client_idx].buf + clients[client_idx].buf_len, tmp_buf, n);
                        clients[client_idx].buf_len += n;
                    }

                    // Обработать все строки по \n
                    while (1) {
                        char *newline = memchr(clients[client_idx].buf, '!', clients[client_idx].buf_len);
                        if (!newline) break;

                        size_t line_len = newline - clients[client_idx].buf + 1;
                        for (size_t j = 0; j < line_len; ++j) {
                            clients[client_idx].buf[j] = toupper((unsigned char)clients[client_idx].buf[j]);
                        }
                        write(STDOUT_FILENO, clients[client_idx].buf, line_len);
                        total_messages++;
                        memmove(clients[client_idx].buf, newline + 1, clients[client_idx].buf_len - line_len);
                        clients[client_idx].buf_len -= line_len;
                    }
                }
            }
        }
    }

    // === Завершение после 10 секунд ===
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double duration = (end_time.tv_sec - start_time.tv_sec) +
                     (end_time.tv_nsec - start_time.tv_nsec) / 1e9;

    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i].fd != -1) {
            close(clients[i].fd);
        }
    }

    close(server_socket);
    close(epoll_fd);
    unlink(SOCKET_PATH);

    printf("\n--- Server finished after %.3f seconds ---\n", duration);
    printf("Total messages processed: %d\n", total_messages);
    return 0;
}