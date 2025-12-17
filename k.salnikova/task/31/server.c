#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <poll.h>
#include <errno.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/uppercase_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

volatile sig_atomic_t stop_server = 0;

void handle_signal(int sig) {
    stop_server = 1;
    printf("\nПолучен сигнал %d, завершаем работу сервера...\n", sig);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    struct pollfd fds[MAX_CLIENTS + 1];
    int nfds = 1;
    int timeout = 1000; 
    char buffer[BUFFER_SIZE];
    int i, bytes_read;
    

    signal(SIGINT, handle_signal); 
    signal(SIGTERM, handle_signal);
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    unlink(SOCKET_PATH);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер запущен и слушает на %s\n", SOCKET_PATH);
    printf("Для завершения работы нажмите Ctrl+C или отправьте SIGTERM\n");
    
    memset(fds, 0, sizeof(fds));
    fds[0].fd = server_fd;
    fds[0].events = POLLIN;
    
    while (!stop_server) {
        int ret = poll(fds, nfds, timeout);
        
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("poll");
            break;
        }
        
        if (ret == 0) {
            continue;
        }
        
        if (fds[0].revents & POLLIN) {
            client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            
            printf("Новый клиент подключен (fd=%d)\n", client_fd);
            
            if (nfds < MAX_CLIENTS + 1) {
                fds[nfds].fd = client_fd;
                fds[nfds].events = POLLIN;
                nfds++;
            } else {
                printf("Достигнуто максимальное число клиентов\n");
                close(client_fd);
            }
        }
        
        for (i = 1; i < nfds; i++) {
            if (fds[i].revents & POLLIN) {
                bytes_read = read(fds[i].fd, buffer, BUFFER_SIZE - 1);
                
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';

                    for (int j = 0; j < bytes_read; j++) {
                        buffer[j] = toupper((unsigned char)buffer[j]);
                    }
                    
                    printf("[Клиент %d]: %s", fds[i].fd, buffer);
                    fflush(stdout);
                }
                
                if (bytes_read <= 0) {
                    printf("Клиент отключился (fd=%d)\n", fds[i].fd);
                    close(fds[i].fd);
                    
                    fds[i].fd = -1;
                }
            }
        }
    
        for (i = 1; i < nfds; i++) {
            if (fds[i].fd == -1) {
                for (int j = i; j < nfds - 1; j++) {
                    fds[j] = fds[j + 1];
                }
                nfds--;
                i--;
            }
        }
    }
    
    printf("Завершаем работу сервера...\n");
    
    for (i = 0; i < nfds; i++) {
        if (fds[i].fd != -1) {
            close(fds[i].fd);
        }
    }
    
    unlink(SOCKET_PATH);
    printf("Сервер завершил работу\n");
    
    return 0;
}