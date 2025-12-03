#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 10

typedef struct {
    int client_fd;
} client_info_t;

static void *handle_client(void *arg) {
    client_info_t *info = (client_info_t *)arg;
    int client_fd = info->client_fd;
    free(info);
    
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        
        // Эхо + префикс сервера
        char response[BUFFER_SIZE * 2];
        snprintf(response, sizeof(response), "echo: %s", buffer);
        
        // Небольшая задержка для демонстрации асинхронности
        usleep(50000); // 50ms
        
        write(client_fd, response, strlen(response));
    }
    
    close(client_fd);
    return NULL;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    
    // Удаляем старый сокет, если существует
    unlink(SOCKET_PATH);
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Настраиваем адрес
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Привязываем сокет
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }
    
    // Слушаем соединения
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    printf("Server listening on %s\n", SOCKET_PATH);
    
    while (1) {
        client_len = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        
        if (client_fd == -1) {
            perror("accept");
            continue;
        }
        
        // Создаем поток для каждого клиента
        pthread_t thread_id;
        client_info_t *info = malloc(sizeof(client_info_t));
        info->client_fd = client_fd;
        
        if (pthread_create(&thread_id, NULL, handle_client, info) != 0) {
            perror("pthread_create");
            free(info);
            close(client_fd);
        }
        
        pthread_detach(thread_id);
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return EXIT_SUCCESS;
}