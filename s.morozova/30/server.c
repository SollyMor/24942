#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#define SOCKET_PATH "/tmp/uppercase_socket"
#define BUFFER_SIZE 1024

volatile sig_atomic_t keep_running = 1;

void signal_handler(int sig) {
    keep_running = 0;
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    time_t start_time, end_time;
    
    // Регистрируем обработчик сигналов
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Засекаем время начала работы сервера
    time(&start_time);
    printf("Сервер запущен в: %s", ctime(&start_time));
    printf("Для остановки сервера нажмите Ctrl+C\n");
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем неблокирующий режим
    fcntl(server_fd, F_SETFL, O_NONBLOCK);
    
    // Удаляем старый сокет файл если существует
    unlink(SOCKET_PATH);
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Сервер слушает на сокете: %s\n", SOCKET_PATH);
    
    // Главный цикл сервера
    while (keep_running) {
        // Принимаем соединение от клиента (неблокирующий)
        client_fd = accept(server_fd, NULL, NULL);
        
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Нет подключений - ждем немного и продолжаем
                usleep(100000); // 100ms
                continue;
            } else {
                perror("accept");
                break;
            }
        }
        
        printf("Клиент подключен\n");
        
        // Обрабатываем данные от клиента
        while (keep_running) {
            bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
            
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0';
                
                // Преобразуем каждый символ в верхний регистр
                for (int i = 0; i < bytes_read; i++) {
                    buffer[i] = toupper(buffer[i]);
                }
                
                // Выводим преобразованный текст
                printf("СЕРВЕР: %s", buffer);
                fflush(stdout);
            } else if (bytes_read == 0) {
                // Клиент отключился
                printf("Клиент отключен\n");
                break;
            } else {
                if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                }
                break;
            }
        }
        
        close(client_fd);
    }
    
    // Завершаем работу сервера
    printf("\nЗавершение работы сервера...\n");
    close(server_fd);
    unlink(SOCKET_PATH);
    
    time(&end_time);
    printf("Сервер завершил работу в: %s", ctime(&end_time));
    printf("Общее время работы сервера: %.2f секунд\n", difftime(end_time, start_time));
    
    return 0;
}