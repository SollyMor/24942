// Напишите две программы, взаимодействующих через Unix domain socket. Первый процесс (сервер) создает сокет и слушает на нем.  При присоединении клиента, сервер получает через соединение текст, состоящий из символов верхнего и нижнего регистров, переводит его в верхний регистр и выводит в свой стандартный поток вывода, аналогично задаче 25. Второй процесс (клиент) устанавливает соединение с сервером и передает ему текст.  После разрыва соединения клиентом, оба процесса завершаются.
// Необходимо обеспечить возможность подключения нескольких клиентов и параллельное (без задержек) получение текста от них.  При этом, преобразованный текст разных клиентов в выдаче сервера может смешиваться.
//Для мультиплексирования соединений клиентов используйте select(3C) или poll(2).
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

static const char *socket_path = "./socket30";

static ssize_t robust_read(int fd, void *buf, size_t count) {
    for (;;) {
        ssize_t n = read(fd, buf, count);
        if (n >= 0) return n;
        if (errno == EINTR) continue;
        return -1;
    }
}

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
    int listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("bind");
        close(listen_fd);
        return 1;
    }

    if (listen(listen_fd, 5) == -1) {
        perror("listen");
        close(listen_fd);
        return 1;
    }

    fd_set all_fds, read_fds;
    int max_fd = listen_fd;
    FD_ZERO(&all_fds);
    FD_SET(listen_fd, &all_fds);

    int had_clients = 0;
    int server_started = 0;
    struct timespec first_event;
    struct timespec last_event;
    char buffer[8192];
    struct timeval timeout;
    struct timeval *timeout_ptr = NULL;
    
    for (;;) {
        read_fds = all_fds;

        // If we had clients and all disconnected, use timeout to wait for new connections
        int active_before = 0;
        for (int fd = listen_fd + 1; fd <= max_fd; ++fd) {
            if (FD_ISSET(fd, &all_fds)) {
                active_before++;
            }
        }
        
        if (had_clients && active_before == 0) {
            timeout.tv_sec = 0;
            timeout.tv_usec = 200000; // 200ms timeout
            timeout_ptr = &timeout;
        } else {
            timeout_ptr = NULL; // Block indefinitely
        }

        int nready = select(max_fd + 1, &read_fds, NULL, NULL, timeout_ptr);
        if (nready == -1) {
            if (errno == EINTR) continue;
            perror("select");
            break;
        }
        
        // If timeout occurred and we had clients but all disconnected, exit
        if (nready == 0 && had_clients && active_before == 0) {
            break;
        }

        // Check for new connections
        if (FD_ISSET(listen_fd, &read_fds)) {
            int client_fd = accept(listen_fd, NULL, NULL);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            FD_SET(client_fd, &all_fds);
            if (client_fd > max_fd) {
                max_fd = client_fd;
            }
            had_clients = 1;
            nready--;
        }

        // Check all client connections for data
        for (int fd = listen_fd + 1; fd <= max_fd && nready > 0; ++fd) {
            if (FD_ISSET(fd, &read_fds)) {
                nready--;
                ssize_t n = robust_read(fd, buffer, sizeof(buffer));
                if (n == 0) {
                    // Client disconnected
                    close(fd);
                    FD_CLR(fd, &all_fds);
                    // Update max_fd if needed
                    if (fd == max_fd) {
                        while (max_fd > listen_fd && !FD_ISSET(max_fd, &all_fds)) {
                            max_fd--;
                        }
                    }
                } else if (n < 0) {
                    perror("read");
                    close(fd);
                    FD_CLR(fd, &all_fds);
                    if (fd == max_fd) {
                        while (max_fd > listen_fd && !FD_ISSET(max_fd, &all_fds)) {
                            max_fd--;
                        }
                    }
                } else {
                    // Process and output data
                    struct timespec start_time, end_time;
                    size_t write_pos = 0;
                    clock_gettime(CLOCK_MONOTONIC, &start_time);
                    for (ssize_t i = 0; i < n; ++i) {
                        unsigned char ch = (unsigned char)buffer[i];
                        // Filter out control characters that might trigger commands
                        if (isalnum(ch) || isprint(ch)) {
                            buffer[write_pos] = (char)(isalpha(ch) ? toupper(ch) : ch);
                            write_pos++;
                        }
                    }
                    n = (ssize_t)write_pos;
                    if (n > 0) {
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

                        // Get current time with millisecond precision and format timestamp
                        struct timeval tv;
                        struct tm *timeinfo;
                        char timestamp[64];
                        
                        gettimeofday(&tv, NULL);
                        timeinfo = localtime(&tv.tv_sec);
                        strftime(timestamp, sizeof(timestamp), "[%Y-%m-%d %H:%M:%S", timeinfo);
                        snprintf(timestamp + strlen(timestamp), sizeof(timestamp) - strlen(timestamp), ".%03ld] ", (long)(tv.tv_usec / 1000));
                        
                        // Write timestamp
                        if (robust_write(STDOUT_FILENO, timestamp, strlen(timestamp)) < 0) {
                            perror("write");
                        }
                        // Write processing info
                        if (robust_write(STDOUT_FILENO, processing_info, strlen(processing_info)) < 0) {
                            perror("write");
                        }
                        // Write processed data
                        if (robust_write(STDOUT_FILENO, buffer, (size_t)n) < 0) {
                            perror("write");
                        }
                        // Add newline after output for next input
                        if (robust_write(STDOUT_FILENO, "\n", 1) < 0) {
                            perror("write");
                        }
                    }
                }
            }
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

    // Cleanup: close all remaining file descriptors
    for (int fd = listen_fd; fd <= max_fd; ++fd) {
        if (FD_ISSET(fd, &all_fds)) {
            close(fd);
        }
    }

    return 0;
}