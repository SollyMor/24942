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

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define DEFAULT_PREFIX "msg"
#define SEND_DURATION_SEC 5.0

static double elapsed_seconds(const struct timeval *start, const struct timeval *current) {
    double seconds = (double)(current->tv_sec - start->tv_sec);
    double useconds = (double)(current->tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

static int send_message_nonblocking(int fd, const char *message) {
    size_t total_written = 0;
    size_t len = strlen(message);

    while (total_written < len) {
        ssize_t written = write(fd, message + total_written, len - total_written);
        if (written == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return 0; // Просто пропускаем это сообщение
            } else if (errno == EPIPE) {
                return -1; // Соединение разорвано
            }
            return -1; // Другая ошибка
        }
        total_written += (size_t)written;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    char message1[BUFFER_SIZE];
    char message2[BUFFER_SIZE];
    struct timeval start_time;
    struct timeval current_time;
    int toggle = 0;
    
    signal(SIGPIPE, SIG_IGN);

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
    
    // Подключаемся к серверу (блокирующий вызов)
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Делаем сокет неблокирующим
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Обрабатываем аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--prefix=", 9) == 0 && strlen(argv[i] + 9) > 0) {
            strncpy(prefix, argv[i] + 9, sizeof(prefix) - 1);
            prefix[sizeof(prefix) - 1] = '\0';
        }
    }

    snprintf(message1, sizeof(message1), "%s1\n", prefix);
    snprintf(message2, sizeof(message2), "%s2\n", prefix);

    gettimeofday(&start_time, NULL);

    while (1) {
        gettimeofday(&current_time, NULL);
        if (elapsed_seconds(&start_time, &current_time) >= SEND_DURATION_SEC) {
            break;
        }
        
        const char *message = toggle ? message2 : message1;
        
        // Просто отправляем, не ждем ответа
        send_message_nonblocking(client_fd, message);
        
        toggle = !toggle;
        usleep(10000); // 10 мс между сообщениями
    }
    
    close(client_fd);
    
    return EXIT_SUCCESS;
}