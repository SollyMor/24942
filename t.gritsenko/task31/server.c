#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>   // socket(), bind(), listen(), accept()
#include <sys/un.h>       // sockaddr_un
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <poll.h>         // poll(), struct pollfd, маски POLLIN/POLLERR/POLLHUP

// Максимальное число ожидающих соединений
#define BACKLOG 5

// Размер массива pollfd = 1 (listen-сокет) + места под клиентов
#define POLL_LENGTH (BACKLOG + 1)

// Функция добавляет новый клиентский сокет в массив poll_fds.
// Возвращает 0 при успехе, -1 если нет свободных слотов.
int addConnection(struct pollfd *poll_list, int fd) {
    int result = -1;

    // Индекс 0 - listen-сокет, поэтому клиенты с индекса 1
    for (int i = 1; i < POLL_LENGTH; i++) {
        // fd < 0 означает свободный слот
        if (poll_list[i].fd < 0) {

            poll_list[i].fd = fd;   // регистрируем нового клиента
            poll_list[i].events = POLLIN | POLLPRI;
            // POLLIN  — обычные данные готовы к чтению
            // POLLPRI — приоритетные данные (редко используются)

            result = 0;   // успех
            break;
        }
    }
    return result;
}

char *socket_path = "./socket";   // путь Unix domain socket (файл-сокет сервера)

int main() {
    char buf[100];
    int fd, cl, rc;

    // Создаём сокет (как TCP, локальный)
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    // Инициализация структуры адреса сокета
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path);   // удаляем старый файл сокета, если он остался

    // Привязываем сокет к файлу socket_path
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(-1);
    }

    // Начинаем слушать входящие подключения
    if (listen(fd, BACKLOG) == -1) {
        perror("listen error");
        exit(-1);
    }

    // Массив pollfd - первый элемент listen-сокет, остальные - клиенты
    struct pollfd poll_fds[POLL_LENGTH];

    for (int i = 0; i < POLL_LENGTH; i++) {
        poll_fds[i].fd = -1;                     // fd < 0 — элемент игнорируется
        poll_fds[i].events = POLLIN | POLLPRI;   // события, которые хотим отслеживать
        // POLLIN  — данные доступны для чтения
        // POLLPRI — приоритетные данные
    }

    poll_fds[0].fd = fd;                         // слот 0: слушающий сокет сервера

    while (1) {
        // Ожидаем события по любому сокету; -1 = ждать бесконечно
        if ((poll(poll_fds, POLL_LENGTH, -1)) == -1) {
            perror("bad poll");
            exit(-1);
        }

        // Проверяем глобальные ошибки типа:
        // POLLERR - ошибка на сокете
        // POLLHUP - клиент закрыл соединение
        // POLLNVAL - сокет недействителен

        for (int i = 0; i < POLL_LENGTH; i++) {
            if (poll_fds[i].fd < 0) continue;

            short revents = poll_fds[i].revents;

            if ((revents & POLLERR) || (revents & POLLHUP) || (revents & POLLNVAL)) {
                close(poll_fds[i].fd);           // закрываем сокет
                poll_fds[i].fd = -1;             // помечаем как свободный

                if (i == 0) {                    // это listen-сокет - серьёзная ошибка
                    printf("Server error\n");
                    exit(-1);
                } else {
                    printf("Closing socket\n");
                }
            }
        }

        // Если listen-сокет получил событие POLLIN — новое подключение.
        if ((poll_fds[0].revents & POLLIN) || (poll_fds[0].revents & POLLPRI)) {

            cl = accept(fd, NULL, NULL);         // принимаем подключение
            if (cl == -1) {
                perror("accept error");
                continue;
            }

            if (addConnection(poll_fds, cl) == -1) {
                perror("Failed to add new connection");
                close(cl);
            }
        }

        // Обрабатываем данные от всех клиентов.
        // Клиенты - элементы 1..POLL_LENGTH-1 массива poll_fds.
        for (int i = 1; i < POLL_LENGTH; i++) {
            if (poll_fds[i].fd < 0) continue;    // пропускаем свободные слоты

            int cur_desc = poll_fds[i].fd;

            // Если клиент прислал данные
            if ((poll_fds[i].revents & POLLIN) || (poll_fds[i].revents & POLLPRI)) {

                rc = read(cur_desc, buf, sizeof(buf));

                if (rc > 0) {
                    // Переводим всё в верхний регистр и выводим на stdout сервера
                    for (int j = 0; j < rc; j++) {
                        buf[j] = toupper(buf[j]);
                        printf("%c", buf[j]);
                    }
                }

                if (rc == -1) {
                    perror("read");
                    exit(-1);

                } else if (rc == 0) {
                    // Клиент закрыл соединение
                    printf("EOF reached, closing connection\n");
                    close(cur_desc);
                    poll_fds[i].fd = -1;
                }
            }
        }
    }
}
