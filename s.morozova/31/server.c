#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <ctype.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/task31_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64

struct timeval program_start_time;

static double elapsed_ms(const struct timeval *start, const struct timeval *end) {
    long sec = end->tv_sec - start->tv_sec;
    long usec = end->tv_usec - start->tv_usec;
    return (double)sec * 1000.0 + (double)usec / 1000.0;
}

void print_total_uptime() {
    struct timeval end_time;
    gettimeofday(&end_time, NULL);
    double total_time_ms = elapsed_ms(&program_start_time, &end_time);
    printf("Время работы: %.3f ms\n", total_time_ms);
}

void signal_handler(int sig) {
    print_total_uptime();
    exit(0);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    char buffer[BUFFER_SIZE];
    ssize_t nbytes;
    
    int client_fds[MAX_CLIENTS];
    int max_fd;
    int i;
    fd_set read_fds;

    // Засекаем время начала работы программы
    gettimeofday(&program_start_time, NULL);
    
    signal(SIGINT, signal_handler);
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    unlink(SOCKET_PATH);
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }

    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;
        
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd) {
                    max_fd = client_fds[i];
                }
            }
        }
        
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            break;
        }
        
        if (FD_ISSET(server_fd, &read_fds)) {
            client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == -1) {
                    client_fds[i] = client_fd;
                    break;
                }
            }
            if (i == MAX_CLIENTS) {
                close(client_fd);
            }
        }
        
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1 && FD_ISSET(client_fds[i], &read_fds)) {
                nbytes = read(client_fds[i], buffer, BUFFER_SIZE - 1);
                
                if (nbytes <= 0) {
                    close(client_fds[i]);
                    client_fds[i] = -1;
                } else {
                    buffer[nbytes] = '\0';
                    for (int j = 0; j < nbytes; j++) {
                        buffer[j] = toupper(buffer[j]);
                    }
                    write(STDOUT_FILENO, buffer, nbytes);
                }
            }
        }
    }
    
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            close(client_fds[i]);
        }
    }
    close(server_fd);
    unlink(SOCKET_PATH);
    
    print_total_uptime();
    return EXIT_SUCCESS;
}