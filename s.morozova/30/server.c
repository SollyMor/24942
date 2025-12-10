#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/select.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define SOCKET_PATH "/tmp/simple_socket"
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 64

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len;
    ssize_t nbytes;
    
    int client_fds[MAX_CLIENTS];
    int i, max_fd;
    fd_set read_fds;
    
    char buffer[BUFFER_SIZE];
    char time_buffer[64];
    
    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Удаляем старый сокет если существует
    unlink(SOCKET_PATH);
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Привязываем сокет
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        exit(EXIT_FAILURE);
    }
    
    // Инициализируем массив клиентов
    for (i = 0; i < MAX_CLIENTS; i++) {
        client_fds[i] = -1;
    }
    
    printf("Server started. Listening on %s\n", SOCKET_PATH);
    
    // Главный цикл сервера
    while (1) {
        FD_ZERO(&read_fds);
        FD_SET(server_fd, &read_fds);
        max_fd = server_fd;
        
        // Добавляем клиентские сокеты
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1) {
                FD_SET(client_fds[i], &read_fds);
                if (client_fds[i] > max_fd) {
                    max_fd = client_fds[i];
                }
            }
        }
        
        // Ждем активности на сокетах
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            if (errno == EINTR) break;
            perror("select");
            break;
        }
        
        // Проверяем новое подключение
        if (FD_ISSET(server_fd, &read_fds)) {
            client_len = sizeof(client_addr);
            client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
            
            if (client_fd == -1) {
                perror("accept");
                continue;
            }
            
            // Ищем свободный слот для клиента
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
        
        // Проверяем данные от клиентов
        for (i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] != -1 && FD_ISSET(client_fds[i], &read_fds)) {
                nbytes = read(client_fds[i], buffer, BUFFER_SIZE - 1);
                
                if (nbytes <= 0) {
                    // Соединение закрыто
                    close(client_fds[i]);
                    client_fds[i] = -1;
                } else {
                    // Получаем текущее время
                    time_t now = time(NULL);
                    struct tm *tm_info = localtime(&now);
                    strftime(time_buffer, sizeof(time_buffer), "[%H:%M:%S] ", tm_info);
                    
                    // Выводим время получения сообщения
                    write(STDOUT_FILENO, time_buffer, strlen(time_buffer));
                    
                    // Преобразуем в верхний регистр
                    for (int j = 0; j < nbytes; j++) {
                        buffer[j] = toupper(buffer[j]);
                    }
                    
                    // Выводим преобразованное сообщение
                    write(STDOUT_FILENO, buffer, nbytes);
                    
                    // Добавляем перенос строки если его нет
                    if (nbytes > 0 && buffer[nbytes-1] != '\n') {
                        write(STDOUT_FILENO, "\n", 1);
                    }
                }
            }
        }
    }
    
    // Закрываем все соединения
    for (i = 0; i < MAX_CLIENTS; i++) {
        if (client_fds[i] != -1) {
            close(client_fds[i]);
        }
    }
    
    close(server_fd);
    unlink(SOCKET_PATH);
    
    return EXIT_SUCCESS;
}