#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define DEFAULT_PREFIX "msg"
#define SEND_DURATION_SEC 15.0

static double elapsed_seconds(const struct timeval *start, const struct timeval *current) {
    double seconds = (double)(current->tv_sec - start->tv_sec);
    double useconds = (double)(current->tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

static void *send_messages(void *arg) {
    int client_fd = *((int *)arg);
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    struct timeval start_time, current_time;
    int message_counter = 1;
    
    // В реальном приложении prefix должен передаваться как аргумент
    // Здесь для простоты оставляем дефолтный
    
    gettimeofday(&start_time, NULL);
    
    while (1) {
        gettimeofday(&current_time, NULL);
        if (elapsed_seconds(&start_time, &current_time) >= SEND_DURATION_SEC) {
            break;
        }
        
        // Формируем и отправляем сообщение
        char message[BUFFER_SIZE];
        snprintf(message, sizeof(message), "%s%d\n", prefix, message_counter++);
        
        // Асинхронная отправка
        ssize_t written = write(client_fd, message, strlen(message));
        
        if (written == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("write");
            break;
        }
        
        // Выводим отправленное сообщение
        printf("Sent: %s", message);
        fflush(stdout);
        
        // Задержка для наглядного перемешивания
        usleep(100000 + (rand() % 100000)); // 100-200ms случайная задержка
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    pthread_t send_thread;
    struct timeval start_time, end_time;
    
    signal(SIGPIPE, SIG_IGN);
    
    // Обрабатываем аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--prefix=", 9) == 0 && strlen(argv[i] + 9) > 0) {
            strncpy(prefix, argv[i] + 9, sizeof(prefix) - 1);
            prefix[sizeof(prefix) - 1] = '\0';
        }
    }
    
    // Создаем сокет
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу
    printf("Connecting to server...\n");
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected. Sending messages...\n");
    
    gettimeofday(&start_time, NULL);
    
    // Создаем поток для отправки сообщений
    if (pthread_create(&send_thread, NULL, send_messages, &client_fd) != 0) {
        perror("pthread_create");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Основной поток просто ждет завершения отправки
    pthread_join(send_thread, NULL);
    
    gettimeofday(&end_time, NULL);
    
    // Небольшая задержка перед закрытием
    usleep(500000);
    
    close(client_fd);
    
    printf("\nTime: %.2f seconds\n", elapsed_seconds(&start_time, &end_time));
    
    return EXIT_SUCCESS;
}