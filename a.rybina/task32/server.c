// Напишите две программы, взаимодействующих через Unix domain socket. Первый процесс (сервер) создает сокет и слушает на нем.  При присоединении клиента, сервер получает через соединение текст, состоящий из символов верхнего и нижнего регистров, переводит его в верхний регистр и выводит в свой стандартный поток вывода, аналогично задаче 25. Второй процесс (клиент) устанавливает соединение с сервером и передает ему текст.  После разрыва соединения клиентом, оба процесса завершаются.
// Необходимо обеспечить возможность подключения нескольких клиентов и параллельное (без задержек) получение текста от них.  При этом, преобразованный текст разных клиентов в выдаче сервера может смешиваться.
// Реализуйте задачу 31, используя асинхронный ввод-вывод вместо select(3C)/poll(2).
// ./server & server_pid=$! && sleep 1 && (./client 1 & ./client 2 & ./client 3 & wait) && kill "$server_pid" && wait "$server_pid" 2>/dev/null

#define _POSIX_C_SOURCE 200809L

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <aio.h>

static const char *socket_path = "/tmp/server_socket";

#define MAX_CLIENTS 100
#define BUFFER_SIZE 8192

typedef struct {
    int fd;
    struct aiocb aio;
    char buffer[BUFFER_SIZE];
    int active;
    int pending;
} client_info_t;

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

int main(void) {
    int server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    unlink(socket_path);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    fd_set read_fds, master_fds;
    FD_ZERO(&master_fds);
    FD_SET(server_fd, &master_fds);

    client_info_t clients[MAX_CLIENTS];
    int had_clients = 0;
    int server_started = 0;
    struct timespec first_event;
    struct timespec last_event;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].active = 0;
        clients[i].pending = 0;
        memset(&clients[i].aio, 0, sizeof(struct aiocb));
    }

    while (1) {
        read_fds = master_fds;
        struct timeval timeout = { .tv_sec = 0, .tv_usec = 100000 }; // 100 ms like reference

        if (select(server_fd + 1, &read_fds, NULL, NULL, &timeout) == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }

        if (FD_ISSET(server_fd, &read_fds)) {
            struct sockaddr_un client_addr;
            socklen_t client_len = sizeof(client_addr);
            int new_client = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            if (new_client == -1) {
                perror("accept");
            } else {
                int placed = 0;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd == -1) {
                        clients[i].fd = new_client;
                        clients[i].active = 1;
                        clients[i].pending = 0;

                        memset(&clients[i].aio, 0, sizeof(struct aiocb));
                        clients[i].aio.aio_fildes = new_client;
                        clients[i].aio.aio_buf = clients[i].buffer;
                        clients[i].aio.aio_nbytes = BUFFER_SIZE - 1;
                        clients[i].aio.aio_offset = 0;

                        if (aio_read(&clients[i].aio) == -1) {
                            perror("aio_read");
                            close(new_client);
                            clients[i].fd = -1;
                            clients[i].active = 0;
                        } else {
                            clients[i].pending = 1;
                            had_clients = 1;
                        }
                        placed = 1;
                        break;
                    }
                }
                if (!placed) {
                    close(new_client);
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1 && clients[i].pending) {
                int error = aio_error(&clients[i].aio);

                if (error == EINPROGRESS) {
                    continue;
                }

                clients[i].pending = 0;

                if (error != 0) {
                    if (error != ECANCELED) {
                        errno = error;
                        perror("aio_error");
                    }
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                    continue;
                }

                ssize_t nbytes = aio_return(&clients[i].aio);
                if (nbytes <= 0) {
                    if (nbytes == -1) {
                        perror("aio_return");
                    }
                    close(clients[i].fd);
                    clients[i].fd = -1;
                    clients[i].active = 0;
                } else {
                    struct timespec start_time, end_time;
                    clock_gettime(CLOCK_MONOTONIC, &start_time);

                    clients[i].buffer[nbytes] = '\0';
                    for (ssize_t j = 0; j < nbytes; ++j) {
                        unsigned char ch = (unsigned char)clients[i].buffer[j];
                        clients[i].buffer[j] = (char)(isalpha(ch) ? toupper(ch) : ch);
                    }

                    if (nbytes > 0) {
                        clock_gettime(CLOCK_MONOTONIC, &end_time);
                        if (!server_started) {
                            first_event = start_time;
                            server_started = 1;
                        }
                        last_event = end_time;
                        long processing_us =
                            (end_time.tv_sec - start_time.tv_sec) * 1000000L +
                            (end_time.tv_nsec - start_time.tv_nsec) / 1000L;
                        char processing_info[64];
                        snprintf(processing_info, sizeof(processing_info),
                                 "[Processing time: %ld us] ", processing_us);

                        struct timeval tv;
                        struct tm *timeinfo;
                        char timestamp[64];
                        gettimeofday(&tv, NULL);
                        timeinfo = localtime(&tv.tv_sec);
                        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S", timeinfo);
                        snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp),
                                 ".%03ld] ", (long)(tv.tv_usec / 1000));

                        if (robust_write(STDOUT_FILENO, timestamp, strlen(timestamp)) < 0) {
                            perror("write");
                        }
                        if (robust_write(STDOUT_FILENO, processing_info, strlen(processing_info)) < 0) {
                            perror("write");
                        }
                        if (robust_write(STDOUT_FILENO, clients[i].buffer, (size_t)nbytes) < 0) {
                            perror("write");
                        }
                        if (robust_write(STDOUT_FILENO, "\n", 1) < 0) {
                            perror("write");
                        }
                    }

                    memset(&clients[i].aio, 0, sizeof(struct aiocb));
                    clients[i].aio.aio_fildes = clients[i].fd;
                    clients[i].aio.aio_buf = clients[i].buffer;
                    clients[i].aio.aio_nbytes = BUFFER_SIZE - 1;
                    clients[i].aio.aio_offset = 0;

                    if (aio_read(&clients[i].aio) == -1) {
                        if (errno != EINPROGRESS) {
                            perror("aio_read");
                            close(clients[i].fd);
                            clients[i].fd = -1;
                            clients[i].active = 0;
                        } else {
                            clients[i].pending = 1;
                        }
                    } else {
                        clients[i].pending = 1;
                    }
                }
            }
        }

        int active_clients = 0;
        int pending_ops = 0;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].fd != -1) {
                active_clients++;
            }
            if (clients[i].pending) {
                pending_ops++;
            }
        }

        if (had_clients && active_clients == 0 && pending_ops == 0) {
            break;
        }
    }

    if (server_started) {
        const char *header = "\n=== SERVER SUMMARY ===\n";
        if (robust_write(STDOUT_FILENO, header, strlen(header)) < 0) {
            perror("write");
        }
        long duration_ms =
            (last_event.tv_sec - first_event.tv_sec) * 1000L +
            (last_event.tv_nsec - first_event.tv_nsec) / 1000000L;
        char duration_buf[128];
        snprintf(duration_buf, sizeof(duration_buf),
                 "[Server active duration: %ld ms]\n", duration_ms);
        if (robust_write(STDOUT_FILENO, duration_buf, strlen(duration_buf)) < 0) {
            perror("write");
        }
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1) {
            aio_cancel(clients[i].fd, NULL);
            close(clients[i].fd);
        }
    }

    close(server_fd);
    unlink(socket_path);
    return 0;
}
