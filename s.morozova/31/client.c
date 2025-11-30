#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024

int client_fd = -1;

void cleanup(int sig) {
    if (client_fd != -1) {
        close(client_fd);
    }
    exit(0);
}

void print_current_time() {
    struct timeval tv;
    struct tm *tm_info;
    char time_buffer[26];
    
    gettimeofday(&tv, NULL);
    tm_info = localtime(&tv.tv_sec);
    
    strftime(time_buffer, 26, "%Y-%m-%d %H:%M:%S", tm_info);
    printf("[%s.%03ld] ", time_buffer, tv.tv_usec / 1000);
}

int main() {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    char response_buffer[BUFFER_SIZE];
    ssize_t bytes_read, bytes_received;
    fd_set read_fds;
    struct timeval timeout;
    
    // Обработчик сигналов для корректного завершения
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Устанавливаем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server. Enter text (Ctrl+C to exit):\n");
    
    while (1) {
        // Выводим приглашение для ввода
        print_current_time();
        printf("Enter text: ");
        fflush(stdout);
        
        // Читаем ввод пользователя
        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            break; // EOF (Ctrl+D) или ошибка
        }
        
        // Замеряем время начала отправки
        struct timeval start_time, end_time;
        gettimeofday(&start_time, NULL);
        
        // Отправляем текст серверу
        if (write(client_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
        
        // Выводим отправленное сообщение с временем
        print_current_time();
        printf("Sent: %s", buffer);
        
        // Ждем ответ от сервера (преобразованный текст)
        FD_ZERO(&read_fds);
        FD_SET(client_fd, &read_fds);
        
        timeout.tv_sec = 5; // Таймаут 5 секунд
        timeout.tv_usec = 0;
        
        int ready = select(client_fd + 1, &read_fds, NULL, NULL, &timeout);
        
        if (ready == -1) {
            perror("select");
            break;
        } else if (ready == 0) {
            print_current_time();
            printf("Timeout waiting for server response\n");
            continue;
        }
        
        if (FD_ISSET(client_fd, &read_fds)) {
            bytes_received = read(client_fd, response_buffer, BUFFER_SIZE - 1);
            if (bytes_received > 0) {
                response_buffer[bytes_received] = '\0';
                
                // Замеряем время получения ответа
                gettimeofday(&end_time, NULL);
                long response_time = (end_time.tv_sec - start_time.tv_sec) * 1000000L + 
                                   (end_time.tv_usec - start_time.tv_usec);
                
                // Выводим полученный ответ с временем исполнения
                print_current_time();
                printf("Received: %s", response_buffer);
                printf("Response time: %ld microseconds\n", response_time);
                
            } else if (bytes_received == 0) {
                print_current_time();
                printf("Server disconnected\n");
                break;
            } else {
                perror("read");
                break;
            }
        }
    }
    
    printf("Disconnecting from server\n");
    close(client_fd);
    
    return 0;
}