// Сервер Unix domain socket:
// создаёт сокет и слушает на нём, принимает несколько клиентов,
// параллельно (без задержек) читает данные через poll(),
// переводит текст в верхний регистр и выводит в stdout (вывод клиентов может смешиваться).

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <time.h>
#include <poll.h>

// Ограничение на число клиентов 
#define MAX_CLIENTS 10
// Путь Unix domain socket, к которому подключаются клиенты
#define SOCKET_PATH "socket31"
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    int active;
    int num;
    char buffer[BUFFER_SIZE];
    ssize_t buf_pos;
    struct timespec connect_time;
} client_t;

static client_t clients[MAX_CLIENTS];
static int server_fd;

static void print_time_event(int num, const char *event) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm tm_info;
    localtime_r(&ts.tv_sec, &tm_info);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm_info);

    printf("[%d] %s %s.%03ld\n", num, event, buf, ts.tv_nsec / 1000000);
    fflush(stdout);
}

static void print_connection_duration(const client_t *c) {
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

// Обработка данных клиента: перевод в верхний регистр и вывод в stdout
static void handle_client_data(const client_t *c) {
    char upper_buf[BUFFER_SIZE];
    int j = 0;

    for (int i = 0; i < c->buf_pos; i++) {
        if (c->buffer[i] == '\n' || c->buffer[i] == '\0') {
            if (j > 0) {
                upper_buf[j] = '\0';
                // Вывод результата в стандартный поток вывода сервера (может смешиваться между клиентами)
                printf("[Client %d] %s\n", c->num, upper_buf);
                fflush(stdout);
                j = 0;
            }

            if (c->buffer[i] == '\n') {
                printf("[Client %d] (empty line)\n", c->num);
                fflush(stdout);
            }
        } else {
            // Преобразование в верхний регистр 
            upper_buf[j++] = (char)toupper((unsigned char)c->buffer[i]);
        }
    }

    if (j > 0) {
        upper_buf[j] = '\0';
        // Вывод результата в stdout
        printf("[Client %d] %s\n", c->num, upper_buf);
        fflush(stdout);
    }
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

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        return 1;
    }

    fcntl(server_fd, F_SETFL, O_NONBLOCK);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].active = 0;
        clients[i].buf_pos = 0;
    }

    printf("Server started. Waiting for connections...\n");
    fflush(stdout);

    while (1) {
        struct pollfd fds[MAX_CLIENTS + 1];
        int nfds = 0;

        // poll(): мультиплексирование — ждём либо новое подключение, либо данные клиентов
        fds[nfds].fd = server_fd;
        fds[nfds].events = POLLIN;
        nfds++;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i].active) {
                fds[nfds].fd = clients[i].fd;
                fds[nfds].events = POLLIN;
                nfds++;
            }
        }

        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            continue;
        }

        // Новый клиент: accept() и добавление в таблицу клиентов
        if (fds[0].revents & POLLIN) {
            int fd = accept(server_fd, NULL, NULL);
            if (fd >= 0) {
                int idx = -1;
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (!clients[i].active) {
                        idx = i;
                        break;
                    }
                }

                if (idx >= 0) {
                    clients[idx].fd = fd;
                    clients[idx].active = 1;
                    clients[idx].num = idx + 1;
                    clients[idx].buf_pos = 0;
                    memset(clients[idx].buffer, 0, sizeof(clients[idx].buffer));

                    clock_gettime(CLOCK_REALTIME, &clients[idx].connect_time);

                    fcntl(fd, F_SETFL, O_NONBLOCK);

                    print_time_event(clients[idx].num, "START");
                    printf("[Client %d] Connected\n", clients[idx].num);
                    fflush(stdout);
                } else {
                    fprintf(stderr, "No free slots for new client\n");
                    close(fd);
                }
            }
        }

        // Параллельная (через poll) обработка данных от нескольких клиентов
        int client_index = 1;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (!clients[i].active) continue;

            if (fds[client_index].revents & POLLIN) {
                ssize_t n = read(clients[i].fd,
                                 clients[i].buffer + clients[i].buf_pos,
                                 BUFFER_SIZE - clients[i].buf_pos - 1);

                if (n > 0) {
                    clients[i].buf_pos += n;
                    clients[i].buffer[clients[i].buf_pos] = '\0';

                    // Перевод в верхний регистр и вывод в stdout
                    handle_client_data(&clients[i]);
                    clients[i].buf_pos = 0;
                } else if (n == 0) {
                    print_connection_duration(&clients[i]);
                    print_time_event(clients[i].num, "END");
                    printf("[Client %d] Disconnected\n", clients[i].num);
                    fflush(stdout);

                    close(clients[i].fd);
                    clients[i].active = 0;
                } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
                    perror("read");
                    close(clients[i].fd);
                    clients[i].active = 0;
                }
            }

            client_index++;
        }
    }

    return 0;
}
