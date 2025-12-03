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
#include <sys/epoll.h>
#include <pthread.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define DEFAULT_PREFIX "msg"
#define SEND_DURATION_SEC 5.0
#define NUM_MESSAGES 10

typedef struct {
    int client_fd;
    char prefix[BUFFER_SIZE];
    int message_counter;
    int message_number;
} client_context_t;

static double elapsed_seconds(const struct timeval *start, const struct timeval *current) {
    double seconds = (double)(current->tv_sec - start->tv_sec);
    double useconds = (double)(current->tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

static void *client_thread(void *arg) {
    client_context_t *ctx = (client_context_t *)arg;
    struct timeval start_time, current_time;
    char message[BUFFER_SIZE];
    
    gettimeofday(&start_time, NULL);
    
    while (1) {
        gettimeofday(&current_time, NULL);
        if (elapsed_seconds(&start_time, &current_time) >= SEND_DURATION_SEC) {
            break;
        }
        
        // Отправляем сообщение
        snprintf(message, sizeof(message), "%s%d\n", ctx->prefix, ctx->message_counter);
        
        // Используем неблокирующую запись
        ssize_t written = write(ctx->client_fd, message, strlen(message));
        
        if (written == -1 && errno != EAGAIN && errno != EWOULDBLOCK) {
            break;
        }
        
        ctx->message_counter++;
        
        // Небольшая задержка для наглядного перемешивания
        usleep(100000); // 100ms
    }
    
    return NULL;
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    pthread_t thread_id;
    client_context_t ctx;
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
    
    // Делаем сокет неблокирующим
    int flags = fcntl(client_fd, F_GETFL, 0);
    fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    
    // Настраиваем адрес сервера
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    // Подключаемся к серверу (асинхронно)
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        if (errno != EINPROGRESS) {
            perror("connect");
            close(client_fd);
            exit(EXIT_FAILURE);
        }
    }
    
    // Ждем подключения
    fd_set write_fds;
    struct timeval timeout;
    FD_ZERO(&write_fds);
    FD_SET(client_fd, &write_fds);
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    
    if (select(client_fd + 1, NULL, &write_fds, NULL, &timeout) <= 0) {
        perror("select on connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Инициализируем контекст для потока
    ctx.client_fd = client_fd;
    strcpy(ctx.prefix, prefix);
    ctx.message_counter = 1;
    
    gettimeofday(&start_time, NULL);
    
    // Создаем поток для отправки сообщений
    if (pthread_create(&thread_id, NULL, client_thread, &ctx) != 0) {
        perror("pthread_create");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Основной поток читает с сервера асинхронно
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    
    // Используем epoll для асинхронного чтения
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = client_fd;
    
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
        perror("epoll_ctl");
        close(epoll_fd);
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    struct epoll_event events[1];
    
    while (1) {
        gettimeofday(&end_time, NULL);
        if (elapsed_seconds(&start_time, &end_time) >= SEND_DURATION_SEC) {
            break;
        }
        
        // Ожидаем события с таймаутом 100ms
        int nfds = epoll_wait(epoll_fd, events, 1, 100);
        
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        
        if (nfds > 0) {
            if (events[0].events & EPOLLIN) {
                bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
                
                if (bytes_read > 0) {
                    buffer[bytes_read] = '\0';
                    // Просто выводим полученное сообщение
                    printf("%s", buffer);
                    fflush(stdout);
                } else if (bytes_read == 0) {
                    // Сервер закрыл соединение
                    break;
                } else if (bytes_read == -1 && errno != EAGAIN) {
                    perror("read");
                    break;
                }
            }
        }
    }
    
    // Ждем завершения потока отправки
    pthread_join(thread_id, NULL);
    
    // Закрываем соединение
    close(epoll_fd);
    close(client_fd);
    
    // Выводим время выполнения
    printf("Time: %.2f seconds\n", elapsed_seconds(&start_time, &end_time));
    
    return EXIT_SUCCESS;
}