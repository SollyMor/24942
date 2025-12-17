#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>     // socket(), bind(), listen(), accept()
#include <sys/un.h>         // sockaddr_un
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>

char *socket_path = "./socket";   // файл Unix-сокета

int main() {
    char buf[100];
    int fd, cl, rc;

    // создаём серверный сокет в домене AF_UNIX, потоковый (как TCP)
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    // инициализация структуры sockaddr_un
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    unlink(socket_path); // удаляем старый файл сокета (если остался)

    // привязываем сокет к файлу socket_path, чтобы клиенты могли подключиться
    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("bind error");
        exit(-1);
    }

    // переводим сокет в режим прослушивания входящих соединений
    if (listen(fd, 5) == -1) {    // 5 - длина очереди ожидающих клиентов
        perror("listen error");
        exit(-1);
    }

    while (1) {
        // принимаем входящее соединение - создаётся новый сокет cl
        if ((cl = accept(fd, NULL, NULL)) == -1) {
            perror("accept error");
            continue;    // ошибка accept - продолжаем ждать других клиентов
        }

        // читаем данные от клиента cl
        while ((rc = read(cl, buf, sizeof(buf))) > 0) {
            // преобразуем каждый символ в верхний регистр и выводим на экран сервера
            for (int i = 0; i < rc; i++) {
                buf[i] = (char) toupper(buf[i]);
                printf("%c", buf[i]);
            }
        }

        // проверка результата чтения
        if (rc == -1) {                            // ошибка чтения
            perror("read");
            exit(-1);
        } else if (rc == 0) {                       // клиент закрыл соединение
            printf("EOF reached, closing connection\n");
            close(cl);                              // закрываем клиентский сокет
        }
    }
}
