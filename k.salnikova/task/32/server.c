#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <signal.h>
#include <aio.h>
#include <fcntl.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/async_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    int fd;
    int active;
    struct aiocb read_aio;
    char read_buffer[BUFFER_SIZE];
} client_t;

client_t clients[MAX_CLIENTS];
int server_fd;
volatile sig_atomic_t stop_server = 0;

void cleanup_server() {
    printf("\nЗавершаем работу сервера...\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            printf("Закрываем клиента %d...\n", clients[i].fd);
            
            int cancel_result = aio_cancel(clients[i].fd, &clients[i].read_aio);
            if (cancel_result == AIO_CANCELED) {
                printf("  AIO операция отменена для клиента %d\n", clients[i].fd);
            } else if (cancel_result == AIO_NOTCANCELED) {
                printf("  AIO операция не может быть отменена для клиента %d\n", clients[i].fd);
            }
            
            close(clients[i].fd);
            clients[i].active = 0;
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    printf("Сервер завершил работу\n");
}

void handle_signal(int sig) {
    stop_server = 1;
}

int find_free_client_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            return i;
        }
    }
    return -1;
}

void process_completed_aio() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            int status = aio_error(&clients[i].read_aio);
            
            if (status == 0) {
                int bytes_read = aio_return(&clients[i].read_aio);
                
                if (bytes_read > 0) {
                    for (int j = 0; j < bytes_read; j++) {
                        clients[i].read_buffer[j] = toupper((unsigned char)clients[i].read_buffer[j]);
                    }
                    clients[i].read_buffer[bytes_read] = '\0';
                    
                    printf("[Клиент %d]: %s", clients[i].fd, clients[i].read_buffer);
                    fflush(stdout);
                    
                    memset(&clients[i].read_aio, 0, sizeof(struct aiocb));
                    clients[i].read_aio.aio_fildes = clients[i].fd;
                    clients[i].read_aio.aio_buf = clients[i].read_buffer;
                    clients[i].read_aio.aio_nbytes = BUFFER_SIZE - 1;
                    
                    if (aio_read(&clients[i].read_aio) == -1) {
                        if (errno != ECANCELED) { 
                            perror("aio_read");
                        }
                        close(clients[i].fd);
                        clients[i].active = 0;
                        printf("Клиент отключился (ошибка чтения)\n");
                    }
                } else {
                    printf("Клиент отключился (fd=%d)\n", clients[i].fd);
                    close(clients[i].fd);
                    clients[i].active = 0;
                }
            } else if (status == ECANCELED) {
                printf("AIO операция отменена для клиента %d\n", clients[i].fd);
                close(clients[i].fd);
                clients[i].active = 0;
            } else if (status != EINPROGRESS) {
                printf("Ошибка AIO для клиента %d: %s\n", clients[i].fd, strerror(status));
                close(clients[i].fd);
                clients[i].active = 0;
            }
        }
    }
}

int main() {
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    fd_set read_fds;
    struct timeval timeout;
    int i;
    
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);
    
    memset(clients, 0, sizeof(clients));
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    int flags = fcntl(server_fd, F_GETFL, 0);
    fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
    
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
    
    printf("Асинхронный сервер запущен и слушает на %s\n", SOCKET_PATH);
    printf("Для завершения работы нажмите Ctrl+C\n");
    
    while (!stop_server) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        int max_fd = server_fd;
        
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                FD_SET(clients[i].fd, &read_fds);
                if (clients[i].fd > max_fd) {
                    max_fd = clients[i].fd;
                }
            }
        }
        
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; 
        
        int ret = select(max_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }
            perror("select");
            break;
        }
        
        if (ret > 0) {
            if (FD_ISSET(server_fd, &read_fds)) {
                client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
                
                if (client_fd == -1) {
                    if (errno != EWOULDBLOCK && errno != EAGAIN) {
                        perror("accept");
                    }
                    continue;
                }
                
                printf("Новый клиент подключен (fd=%d)\n", client_fd);
                
                int slot = find_free_client_slot();
                if (slot != -1) {
                    clients[slot].fd = client_fd;
                    clients[slot].active = 1;
                    
                    memset(&clients[slot].read_aio, 0, sizeof(struct aiocb));
                    clients[slot].read_aio.aio_fildes = client_fd;
                    clients[slot].read_aio.aio_buf = clients[slot].read_buffer;
                    clients[slot].read_aio.aio_nbytes = BUFFER_SIZE - 1;
                    
                    if (aio_read(&clients[slot].read_aio) == -1) {
                        perror("aio_read");
                        close(client_fd);
                        clients[slot].active = 0;
                        printf("Ошибка запуска асинхронного чтения\n");
                    } else {
                        printf("Асинхронное чтение запущено для клиента %d\n", client_fd);
                    }
                } else {
                    printf("Достигнуто максимальное число клиентов\n");
                    close(client_fd);
                }
            }
            
            for (i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].active && FD_ISSET(clients[i].fd, &read_fds)) {
                    char buf[1];
                    int n = recv(clients[i].fd, buf, 1, MSG_PEEK | MSG_DONTWAIT);
                    if (n == 0) {
                        printf("Клиент отключился (обнаружено в select)\n");
                        close(clients[i].fd);
                        clients[i].active = 0;
                    }
                }
            }
        }
        process_completed_aio();
    }
    
    cleanup_server();
    
    return 0;
}