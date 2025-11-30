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
pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void print_current_time() {
    struct timeval tv;
    struct tm *tm_info;
    char time_buffer[26];
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s.%03ld] ", time_buffer, tv.tv_usec / 1000);
}

// Функция для поиска клиента по файловому дескриптору
client_t* find_client_by_fd(int fd) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd == fd) {
            return &clients[i];
        }
    }
    return NULL;
}

void aio_completion_handler(union sigval sigval) {
    struct aiocb *req = (struct aiocb *)sigval.sival_ptr;
    int fd = req->aio_fildes;
    
    pthread_mutex_lock(&clients_mutex);
    client_t *client = find_client_by_fd(fd);
    if (client == NULL) {
        pthread_mutex_unlock(&clients_mutex);
        return;
    }
    
    int bytes_read = aio_return(req);
    
    if (bytes_read > 0) {
        // Копируем данные чтобы избежать гонки
        char temp_buffer[BUFFER_SIZE];
        char output_buffer[BUFFER_SIZE];
        memcpy(temp_buffer, client->buffer, bytes_read);
        temp_buffer[bytes_read] = '\0';
        
        // Преобразуем в верхний регистр
        for (int i = 0; i < bytes_read; i++) {
            output_buffer[i] = toupper(temp_buffer[i]);
        }
        output_buffer[bytes_read] = '\0';
        
        // Блокируем вывод для атомарной записи
        pthread_mutex_lock(&output_mutex);
        print_current_time();
        printf("клиент %d: %s", client->client_id, output_buffer);
        fflush(stdout);
        pthread_mutex_unlock(&output_mutex);
        
        // Отправляем ответ обратно клиенту
        write(client->fd, output_buffer, bytes_read);
        
        // Перезапускаем асинхронное чтение
        memset(&client->aio_cb, 0, sizeof(struct aiocb));
        client->aio_cb.aio_fildes = client->fd;
        client->aio_cb.aio_buf = client->buffer;
        client->aio_cb.aio_nbytes = BUFFER_SIZE - 1;
        client->aio_cb.aio_offset = 0;
        
        client->aio_cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
        client->aio_cb.aio_sigevent.sigev_notify_function = aio_completion_handler;
        client->aio_cb.aio_sigevent.sigev_value.sival_ptr = &client->aio_cb;
        
        if (aio_read(&client->aio_cb) == -1) {
            perror("aio_read");
            client->active = 0;
            close(client->fd);
        }
        
    } else if (bytes_read == 0) {
        // Клиент отключился
        pthread_mutex_lock(&output_mutex);
        print_current_time();
        printf("клиент %d отключился\n", client->client_id);
        pthread_mutex_unlock(&output_mutex);
        
        close(client->fd);
        client->active = 0;
        client->fd = -1;
    } else {
        perror("aio_read error");
        close(client->fd);
        client->active = 0;
        client->fd = -1;
    }
    
    pthread_mutex_unlock(&clients_mutex);
}

int find_free_client_slot() {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            return i;
        }
    }
    return -1;
}

void setup_aio_for_client(client_t *client) {
    // Настраиваем асинхронное чтение
    memset(&client->aio_cb, 0, sizeof(struct aiocb));
    client->aio_cb.aio_fildes = client->fd;
    client->aio_cb.aio_buf = client->buffer;
    client->aio_cb.aio_nbytes = BUFFER_SIZE - 1;
    client->aio_cb.aio_offset = 0;
    
    // Настраиваем обработчик завершения
    client->aio_cb.aio_sigevent.sigev_notify = SIGEV_THREAD;
    client->aio_cb.aio_sigevent.sigev_notify_function = aio_completion_handler;
    client->aio_cb.aio_sigevent.sigev_value.sival_ptr = &client->aio_cb;
    
    // Запускаем асинхронное чтение
    if (aio_read(&client->aio_cb) == -1) {
        perror("aio_read");
        client->active = 0;
        close(client->fd);
    }
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
        
        pthread_mutex_lock(&clients_mutex);
        int client_slot = find_free_client_slot();
        if (client_slot == -1) {
            pthread_mutex_lock(&output_mutex);
            print_current_time();
            printf("Нет свободных слотов для нового клиента\n");
            pthread_mutex_unlock(&output_mutex);
            close(new_client_fd);
            pthread_mutex_unlock(&clients_mutex);
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
        
        // Настраиваем асинхронное чтение для нового клиента
        setup_aio_for_client(&clients[client_slot]);
        pthread_mutex_unlock(&clients_mutex);
        
        // Очищаем неактивных клиентов
        pthread_mutex_lock(&clients_mutex);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active && clients[i].fd == -1) {
                clients[i].active = 0;
            }
        }
        pthread_mutex_unlock(&clients_mutex);
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return 0;
}