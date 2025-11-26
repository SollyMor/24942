#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/epoll.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>

#define SOCKET_PATH "/tmp/case_converter_epoll_socket"
#define BUFFER_SIZE 1024
#define MAX_EVENTS 10
#define MAX_CLIENTS 10

typedef struct {
    int fd;
    char read_buffer[BUFFER_SIZE];
    int active;
    int client_id;
} client_t;

client_t clients[MAX_CLIENTS];
int server_fd;
int epoll_fd;
int client_counter = 0;

void cleanup() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].fd);
        }
    }
    
    if (epoll_fd >= 0) close(epoll_fd);
    if (server_fd >= 0) close(server_fd);
    unlink(SOCKET_PATH);
}

void handle_client_data(int client_fd) {
    client_t *client = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd == client_fd) {
            client = &clients[i];
            break;
        }
    }
    
    if (!client) return;

    ssize_t bytes_read = read(client_fd, client->read_buffer, BUFFER_SIZE - 1);
    
    if (bytes_read > 0) {
        printf("[Клиент %d]: ", client->client_id);
        for (int i = 0; i < bytes_read; i++) {
            putchar(toupper(client->read_buffer[i]));
        }
        fflush(stdout);
        
    } else if (bytes_read == 0) {
        printf("Клиент %d отключен\n", client->client_id);
        
        struct epoll_event ev;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);
        
        close(client_fd);
        client->active = 0;
        
    } else {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            struct epoll_event ev;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);
            close(client_fd);
            client->active = 0;
        }
    }
}

void accept_new_client() {
    int new_socket = accept(server_fd, NULL, NULL);
    if (new_socket == -1) {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("accept");
        }
        return;
    }
    
    int flags = fcntl(new_socket, F_GETFL, 0);
    if (fcntl(new_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(new_socket);
        return;
    }
    
    int added = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd = new_socket;
            clients[i].active = 1;
            clients[i].client_id = ++client_counter;
            
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;
            ev.data.fd = new_socket;
            
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                perror("epoll_ctl");
                close(new_socket);
                clients[i].active = 0;
            } else {
                printf("Клиент %d подключен\n", clients[i].client_id);
                added = 1;
            }
            break;
        }
    }
    
    if (!added) {
        close(new_socket);
    }
}

int main() {
    struct sockaddr_un server_addr;
    struct epoll_event events[MAX_EVENTS];
    
    atexit(cleanup);
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].fd = -1;
    }
    
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
        close(server_fd);
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
    
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер запущен\n");
    
    while (1) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                accept_new_client();
            } else {
                handle_client_data(events[i].data.fd);
            }
        }
    }
    
    return 0;
}