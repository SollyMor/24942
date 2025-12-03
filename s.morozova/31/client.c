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
#define MESSAGE_COUNT 30
#define DELAY_MICROSECONDS 1000  // 1 миллисекунда для наглядности

int client_fd = -1;

void cleanup(int sig) {
    if (client_fd != -1) {
        close(client_fd);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    char response_buffer[BUFFER_SIZE];
    struct timespec sleep_time;
    struct timeval start_time, end_time;
    long total_response_time = 0;
    int successful_messages = 0;
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <text>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    strncpy(buffer, argv[1], BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    
    if (buffer[strlen(buffer) - 1] != '\n') {
        strncat(buffer, "\n", BUFFER_SIZE - strlen(buffer) - 1);
    }
    
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
    // Настраиваем время задержки
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = DELAY_MICROSECONDS * 1000; // микросекунды в наносекунды
    
    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    printf("Подключаюсь к серверу...\n");
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Подключено. Отправляю текст: %s", buffer);
    printf("Буду отправлять %d сообщений с задержкой %d микросекунд\n\n", 
           MESSAGE_COUNT, DELAY_MICROSECONDS);
    
    for (int i = 0; i < MESSAGE_COUNT; i++) {
        // Засекаем время начала отправки
        gettimeofday(&start_time, NULL);
        
        // Отправляем сообщение (синхронно)
        if (write(client_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
        
        printf("Отправлено сообщение %d/%d\n", i+1, MESSAGE_COUNT);
        
        // Ждем ответ от сервера (синхронно)
        ssize_t bytes_received = read(client_fd, response_buffer, BUFFER_SIZE - 1);
        
        if (bytes_received > 0) {
            response_buffer[bytes_received] = '\0';
            
            // Засекаем время получения ответа
            gettimeofday(&end_time, NULL);
            
            // Вычисляем время отклика
            long response_time = (end_time.tv_sec - start_time.tv_sec) * 1000000L + 
                               (end_time.tv_usec - start_time.tv_usec);
            total_response_time += response_time;
            successful_messages++;
            
            printf("  Получен ответ: %s", response_buffer);
            printf("  Время отклика: %ld микросекунд\n", response_time);
            
        } else if (bytes_received == 0) {
            printf("  Сервер отключился\n");
            break;
        } else {
            perror("  Ошибка чтения");
            break;
        }
        
        // Задержка между сообщениями
        nanosleep(&sleep_time, NULL);
    }
    
    // Выводим статистику клиента
    printf("\n=== Статистика клиента ===\n");
    printf("Успешно отправлено сообщений: %d/%d\n", successful_messages, MESSAGE_COUNT);
    if (successful_messages > 0) {
        printf("Среднее время отклика: %.2f микросекунд\n", 
               (double)total_response_time / successful_messages);
    }
    printf("Отключаюсь от сервера\n");
    
    close(client_fd);
    
    return 0;
}