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
#include <aio.h>

#define SOCKET_PATH "/tmp/task32_socket"
#define BUFFER_SIZE 1024
#define DEFAULT_PREFIX "msg"
#define SEND_DURATION_SEC 5.0
#define MAX_PENDING_OPS 10

static double elapsed_seconds(const struct timeval *start, const struct timeval *current) {
    double seconds = (double)(current->tv_sec - start->tv_sec);
    double useconds = (double)(current->tv_usec - start->tv_usec) / 1000000.0;
    return seconds + useconds;
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char prefix[BUFFER_SIZE] = DEFAULT_PREFIX;
    struct timeval start_time;
    struct timeval current_time;
    
    struct aiocb aio_ops[MAX_PENDING_OPS];
    char buffers[MAX_PENDING_OPS][BUFFER_SIZE];
    int pending_count = 0;
    
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
    
    // Подключаемся к серверу
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    // Обрабатываем аргументы командной строки
    for (int i = 1; i < argc; ++i) {
        if (strncmp(argv[i], "--prefix=", 9) == 0 && strlen(argv[i] + 9) > 0) {
            strncpy(prefix, argv[i] + 9, sizeof(prefix) - 1);
            prefix[sizeof(prefix) - 1] = '\0';
        }
    }

    gettimeofday(&start_time, NULL);
    int message_counter = 0;

    while (1) {
        gettimeofday(&current_time, NULL);
        if (elapsed_seconds(&start_time, &current_time) >= SEND_DURATION_SEC) {
            break;
        }
        
        // Если есть свободные слоты для AIO операций
        if (pending_count < MAX_PENDING_OPS) {
            // Подготавливаем сообщение
            int idx = pending_count;
            snprintf(buffers[idx], BUFFER_SIZE, "%s%d\n", prefix, ++message_counter);
            
            // Настраиваем асинхронную операцию записи
            memset(&aio_ops[idx], 0, sizeof(struct aiocb));
            aio_ops[idx].aio_fildes = client_fd;
            aio_ops[idx].aio_buf = buffers[idx];
            aio_ops[idx].aio_nbytes = strlen(buffers[idx]);
            aio_ops[idx].aio_offset = 0;
            aio_ops[idx].aio_sigevent.sigev_notify = SIGEV_NONE;
            
            // Запускаем асинхронную запись
            if (aio_write(&aio_ops[idx]) == 0) {
                pending_count++;
            }
        }
        
        // Проверяем завершенные операции
        for (int i = 0; i < pending_count; i++) {
            int status = aio_error(&aio_ops[i]);
            
            if (status == 0) {
                // Операция завершена
                ssize_t result = aio_return(&aio_ops[i]);
                if (result > 0) {
                    // Успешно отправлено
                } else if (result == 0) {
                    // Сервер закрыл соединение
                    close(client_fd);
                    return EXIT_SUCCESS;
                }
                
                // Удаляем завершенную операцию из массива
                for (int j = i; j < pending_count - 1; j++) {
                    aio_ops[j] = aio_ops[j + 1];
                    memcpy(buffers[j], buffers[j + 1], BUFFER_SIZE);
                }
                pending_count--;
                i--;
            } else if (status != EINPROGRESS) {
                // Ошибка
                perror("aio_write error");
                close(client_fd);
                return EXIT_FAILURE;
            }
        }
        
        usleep(1000); // Небольшая пауза
    }
    
    // Ждем завершения всех оставшихся операций
    while (pending_count > 0) {
        for (int i = 0; i < pending_count; i++) {
            int status = aio_error(&aio_ops[i]);
            
            if (status == 0) {
                aio_return(&aio_ops[i]);
                
                for (int j = i; j < pending_count - 1; j++) {
                    aio_ops[j] = aio_ops[j + 1];
                    memcpy(buffers[j], buffers[j + 1], BUFFER_SIZE);
                }
                pending_count--;
                i--;
            }
        }
        usleep(1000);
    }
    
    close(client_fd);
    
    return EXIT_SUCCESS;
}