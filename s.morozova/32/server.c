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
    time_t connect_time;
} client_t;

client_t clients[MAX_CLIENTS];
int server_fd;
int epoll_fd;
int client_counter = 0;

void print_status() {
    printf("\n=== ТЕКУЩЕЕ СОСТОЯНИЕ СЕРВЕРА ===\n");
    printf("Время: %ld\n", time(NULL));
    printf("Активные клиенты:\n");
    
    int active_count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            printf("  Клиент %d (fd: %d) - подключен %ld сек назад\n", 
                   clients[i].client_id, clients[i].fd, 
                   time(NULL) - clients[i].connect_time);
            active_count++;
        }
    }
    printf("Всего активных: %d/%d\n", active_count, MAX_CLIENTS);
    printf("================================\n\n");
}

void cleanup() {
    printf("\nОчистка ресурсов...\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active) {
            close(clients[i].fd);
            clients[i].active = 0;
        }
    }
    
    if (epoll_fd >= 0) {
        close(epoll_fd);
    }
    
    if (server_fd >= 0) {
        close(server_fd);
    }
    unlink(SOCKET_PATH);
}

void handle_client_data(int client_fd) {
    // Ищем клиента в массиве
    client_t *client = NULL;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].active && clients[i].fd == client_fd) {
            client = &clients[i];
            break;
        }
    }
    
    if (!client) {
        return;
    }
    
    // Имитируем обработку с задержкой для демонстрации асинхронности
    printf("[ОБРАБОТКА] Начало обработки данных от клиента %d\n", client->client_id);
    
    // Читаем данные (неблокирующее чтение)
    ssize_t bytes_read = read(client_fd, client->read_buffer, BUFFER_SIZE - 1);
    
    if (bytes_read > 0) {
        client->read_buffer[bytes_read] = '\0';
        
        // Имитируем "тяжелую" обработку
        printf("[ОБРАБОТКА] Клиент %d отправил %ld байт. Обрабатываю...\n", 
               client->client_id, bytes_read);
        
        // Преобразуем в верхний регистр и выводим
        printf("[РЕЗУЛЬТАТ Клиент %d]: ", client->client_id);
        for (int i = 0; i < bytes_read; i++) {
            char c = toupper(client->read_buffer[i]);
            putchar(c);
            fflush(stdout); // Немедленный вывод
            usleep(50000); // Задержка 50ms для наглядности
        }
        printf("\n");
        
        printf("[ОБРАБОТКА] Завершена обработка данных от клиента %d\n", client->client_id);
        
    } else if (bytes_read == 0) {
        // Соединение закрыто клиентом
        printf("[ОТКЛЮЧЕНИЕ] Клиент %d отключился (fd: %d)\n", client->client_id, client_fd);
        
        // Удаляем из epoll
        struct epoll_event ev;
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);
        
        close(client_fd);
        client->active = 0;
        
        print_status();
        
    } else {
        if (errno != EWOULDBLOCK && errno != EAGAIN) {
            perror("read error");
            
            // Удаляем из epoll
            struct epoll_event ev;
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, &ev);
            
            close(client_fd);
            client->active = 0;
            
            print_status();
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
    
    // Устанавливаем неблокирующий режим для нового клиента
    int flags = fcntl(new_socket, F_GETFL, 0);
    if (fcntl(new_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl client O_NONBLOCK");
        close(new_socket);
        return;
    }
    
    // Ищем свободный слот для нового клиента
    int added = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i].active) {
            clients[i].fd = new_socket;
            clients[i].active = 1;
            clients[i].client_id = ++client_counter;
            clients[i].connect_time = time(NULL);
            
            // Добавляем клиента в epoll
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET; // Чтение + edge-triggered режим
            ev.data.fd = new_socket;
            
            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, new_socket, &ev) == -1) {
                perror("epoll_ctl add client");
                close(new_socket);
                clients[i].active = 0;
            } else {
                printf("[ПОДКЛЮЧЕНИЕ] Новый клиент %d подключен (fd: %d)\n", 
                       clients[i].client_id, new_socket);
                added = 1;
                
                // Отправляем приветственное сообщение
                const char *welcome_msg = "Добро пожаловать! Отправьте текст для преобразования в верхний регистр.\n";
                write(new_socket, welcome_msg, strlen(welcome_msg));
                
                print_status();
            }
            break;
        }
    }
    
    if (!added) {
        printf("[ОШИБКА] Достигнут лимит клиентов (%d), отказываем в подключении\n", MAX_CLIENTS);
        const char *reject_msg = "Сервер перегружен. Попробуйте позже.\n";
        write(new_socket, reject_msg, strlen(reject_msg));
        close(new_socket);
    }
}

int main() {
    struct sockaddr_un server_addr;
    struct epoll_event events[MAX_EVENTS];
    
    // Регистрируем обработчик очистки
    atexit(cleanup);
    
    // Инициализируем структуры клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].fd = -1;
        clients[i].client_id = 0;
    }
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем неблокирующий режим для серверного сокета
    int flags = fcntl(server_fd, F_GETFL, 0);
    if (fcntl(server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl O_NONBLOCK");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Удаляем старый сокет если он существует
    unlink(SOCKET_PATH);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем прослушивание
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Создаем epoll instance
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Добавляем серверный сокет в epoll
    struct epoll_event ev;
    ev.events = EPOLLIN; // Нас интересуют события чтения
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl add server");
        close(epoll_fd);
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("=== АСИНХРОННЫЙ СЕРВЕР ЗАПУЩЕН ===\n");
    printf("Сокет: %s\n", SOCKET_PATH);
    printf("Максимум клиентов: %d\n", MAX_CLIENTS);
    printf("Режим: Edge-Triggered (ET)\n");
    printf("Ожидание подключений...\n\n");
    
    // Основной цикл epoll
    while (1) {
        // Ждем события (блокирующий вызов)
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        
        printf("[EPOLL] Обрабатываю %d событий\n", nfds);
        
        // Обрабатываем все готовые события
        for (int i = 0; i < nfds; i++) {
            if (events[i].data.fd == server_fd) {
                // Новое подключение на серверном сокете
                printf("[СОБЫТИЕ] Новое подключение\n");
                accept_new_client();
            } else {
                // Данные от клиента готовы к чтению
                printf("[СОБЫТИЕ] Данные от клиента (fd: %d)\n", events[i].data.fd);
                handle_client_data(events[i].data.fd);
            }
        }
    }
    
    return 0;
}