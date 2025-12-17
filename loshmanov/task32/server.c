// Сервер Unix domain socket (аналог задачи 31):
// принимает несколько клиентов, получает текст, переводит в верхний регистр и печатает в stdout.
// Отличие задачи 32: вместо select/poll используется асинхронный ввод-вывод (POSIX AIO) с aiocb.

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
// Асинхронный ввод-вывод (AIO) вместо poll/select
#include <aio.h>

#define NUM_CLIENTS 10
#define SOCKET_PATH "socket32"
#define BLOCK_SIZE 4

typedef struct {
    int fd;
    int active;
    int num;
    // Контекст асинхронного чтения для клиента
    struct aiocb aio;
    char buf[BLOCK_SIZE + 1];
    struct timespec connect_time;
} client_t;

client_t clients[NUM_CLIENTS];
int server_fd;

void print_time_event(int num, const char *event) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);
    printf("[%d] %s %s.%03ld\n", num, event, buf, ts.tv_nsec / 1000000);
    fflush(stdout);
}

void print_connection_duration(client_t *c) {
    struct timespec current_time;
    clock_gettime(CLOCK_REALTIME, &current_time);

    long seconds = current_time.tv_sec - c->connect_time.tv_sec;
    long nanoseconds = current_time.tv_nsec - c->connect_time.tv_nsec;

    if (nanoseconds < 0) {
        seconds--;
        nanoseconds += 1000000000L;
    }

    long milliseconds = nanoseconds / 1000000;

    printf("[Client %d] Was on server for: %ld.%03ld seconds\n",
           c->num, seconds, milliseconds);
    fflush(stdout);
}

// Запуск асинхронного чтения для конкретного клиента (AIO вместо poll/select)
void start_aio(client_t *c) {
    memset(&c->aio, 0, sizeof(c->aio));
    c->aio.aio_fildes = c->fd;
    c->aio.aio_buf = c->buf;
    c->aio.aio_nbytes = BLOCK_SIZE;
    aio_read(&c->aio);
}

int main(void) {
    // Создание серверного Unix domain stream socket и перевод в режим прослушивания
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    unlink(SOCKET_PATH);

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, NUM_CLIENTS) < 0) {
        perror("listen");
        return 1;
    }

    for (int i = 0; i < NUM_CLIENTS; i++)
        clients[i].active = 0;

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    while (1) {
        // Приём новых клиентов (accept) без блокировки
        int fd = accept(server_fd, NULL, NULL);
        if (fd >= 0) {
            int idx = -1;
            for (int i = 0; i < NUM_CLIENTS; i++)
                if (!clients[i].active) {
                    idx = i;
                    break;
                }

            if (idx >= 0) {
                clients[idx].fd = fd;
                clients[idx].active = 1;
                clients[idx].num = idx + 1;

                clock_gettime(CLOCK_REALTIME, &clients[idx].connect_time);

                print_time_event(clients[idx].num, "START");
                fcntl(fd, F_SETFL, O_NONBLOCK);

                // Старт асинхронного чтения от клиента
                start_aio(&clients[idx]);
            } else {
                fprintf(stderr, "No free slots for new client\n");
                close(fd);
            }
        }

        // Параллельная обработка клиентов: проверяем завершение AIO-операций и печатаем результат
        for (int i = 0; i < NUM_CLIENTS; i++) {
            if (!clients[i].active)
                continue;

            // AIO состояние: 0 => чтение завершилось; EINPROGRESS => ещё выполняется
            int err = aio_error(&clients[i].aio);
            if (err == 0) {
                int n = aio_return(&clients[i].aio);

                if (n > 0) {
                    clients[i].buf[n] = 0;

                    // Преобразование текста клиента в верхний регистр 
                    for (int j = 0; j < n; j++)
                        clients[i].buf[j] = toupper((unsigned char)clients[i].buf[j]);

                    // Вывод в stdout; строки разных клиентов могут смешиваться 
                    printf("[%d] %s", i, clients[i].buf);
                    fflush(stdout);

                    // Перезапуск асинхронного чтения для дальнейших данных
                    start_aio(&clients[i]);
                } else if (n == 0) {
                    // Клиент закрыл соединение
                    print_connection_duration(&clients[i]);
                    print_time_event(clients[i].num, "END");

                    close(clients[i].fd);
                    clients[i].active = 0;
                }
            } else if (err != EINPROGRESS) {
                // Ошибка AIO — закрываем клиента
                perror("aio_error");
                close(clients[i].fd);
                clients[i].active = 0;
            }
        }

        usleep(1000);
    }

    return 0;
}
