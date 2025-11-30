#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>
#include <aio.h>
#include <signal.h>
#include <pthread.h>
#include <asm-generic/siginfo.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int client_id;
    int active;
    struct aiocb aio_cb;
    char buffer[BUFFER_SIZE];
    char output_buffer[BUFFER_SIZE];
} client_t;

client_t clients[MAX_CLIENTS];
int next_client_id = 0;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_current_time() {
    struct timeval tv;
    struct tm *tm_info;
    char time_buffer[26];
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s.%03ld] ", time_buffer, tv.tv_usec / 1000);
}

void aio_completion_handler(sigval_t sigval) {
    struct aiocb *aio_cb = (struct aiocb *)sigval.sival_ptr;
    client_t *client = (client_t *)aio_cb->aio_sigevent.sigev_value.sival_ptr;
    
    int bytes_read = aio_return(aio_cb);
    
    if (bytes_read > 0) {
        client->buffer[bytes_read] = '\0';
        
        // Преобразуем в верхний регистр
        for (int i = 0; i < bytes_read; i++) {
            client->output_buffer[i] = toupper(client->buffer[i]);
        }
        client->output_buffer[bytes_read] = '\0';
        
        // Блокируем вывод для атомарной записи
        pthread_mutex_lock(&output_mutex);
        
        print_current_time();
        printf("клиент %d: %s", client->client_id, client->output_buffer);
        fflush(stdout);
        
        pthread_mutex_unlock(&output_mutex);
        
        // Отправляем ответ обратно клиенту
        write(client->fd, client->output_buffer, bytes_read);
        
        // Запускаем следующее асинхронное чтение
        memset(&client->aio_cb, 0, sizeof(struct aiocb));
        client->aio_cb.aio_fildes = client->fd;
        client->aio_cb.aio_buf = client->buffer;
        client->aio_cb.aio_nbytes = BUFFER_SIZE - 1;
        client->aio_cb.aio_offset = 0;
        
        // Настраиваем обработчик завершения
        client->aio_cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
        client->aio_cb.aio_sigevent.sigev_notify_function = aio_completion_handler;
        client->aio_cb.aio_sigevent.sigev_value.sival_ptr = &client->aio_cb;
        client->aio_cb.aio_sigevent.sigev_notify_attributes = NULL;
        
        if (aio_read(&client->aio_cb) == -1) {
            perror("aio_read");
            client->active = 0;
        }
        
    } else if (bytes_read == 0) {
        // Клиент отключился
        pthread_mutex_lock(&output_mutex);
        print_current_time();
        printf("клиент %d отключился\n", client->client_id);
        pthread_mutex_unlock(&output_mutex);
        
        close(client->fd);
        client->active = 0;
    } else {
        perror("aio_read error");
        client->active = 0;
    }
}

int find_free_client_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            return i;
        }
    }
    return -1;
}

int main() {
    int server_fd, new_client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    
    // Инициализация массива клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].fd = -1;
    }
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Удаляем старый сокет файл если существует
    unlink(SOCKET_PATH);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, MAX_CLIENTS) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер слушает на сокете: %s\n", SOCKET_PATH);
    printf("Ожидание подключений...\n");
    
    while (1) {
        client_len = sizeof(client_addr);
        new_client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (new_client_fd == -1) {
            perror("accept");
            continue;
        }
        
        int client_slot = find_free_client_slot();
        if (client_slot == -1) {
            pthread_mutex_lock(&output_mutex);
            print_current_time();
            printf("Нет свободных слотов для нового клиента\n");
            pthread_mutex_unlock(&output_mutex);
            close(new_client_fd);
            continue;
        }
        
        // Инициализируем клиента
        clients[client_slot].fd = new_client_fd;
        clients[client_slot].client_id = next_client_id++;
        clients[client_slot].active = 1;
        
        pthread_mutex_lock(&output_mutex);
        print_current_time();
        printf("клиент %d подключился\n", clients[client_slot].client_id);
        pthread_mutex_unlock(&output_mutex);
        
        // Настраиваем асинхронное чтение
        memset(&clients[client_slot].aio_cb, 0, sizeof(struct aiocb));
        clients[client_slot].aio_cb.aio_fildes = new_client_fd;
        clients[client_slot].aio_cb.aio_buf = clients[client_slot].buffer;
        clients[client_slot].aio_cb.aio_nbytes = BUFFER_SIZE - 1;
        clients[client_slot].aio_cb.aio_offset = 0;
        
        // Настраиваем обработчик завершения
        clients[client_slot].aio_cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
        clients[client_slot].aio_cb.aio_sigevent.sigev_notify_function = aio_completion_handler;
        clients[client_slot].aio_cb.aio_sigevent.sigev_value.sival_ptr = &clients[client_slot].aio_cb;
        clients[client_slot].aio_cb.aio_sigevent.sigev_value.sival_ptr = &clients[client_slot];
        clients[client_slot].aio_cb.aio_sigevent.sigev_notify_attributes = NULL;
        
        // Запускаем асинхронное чтение
        if (aio_read(&clients[client_slot].aio_cb) == -1) {
            perror("aio_read");
            clients[client_slot].active = 0;
            close(new_client_fd);
        }
        
        // Очищаем неактивных клиентов
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && clients[i].fd == -1) {
                clients[i].active = 0;
            }
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}