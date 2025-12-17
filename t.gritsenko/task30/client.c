#include <sys/socket.h>   // функции socket(), connect(), write(), read()
#include <sys/un.h>       // структура sockaddr_un
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>

char *socket_path = "./socket";   // путь к файлу Unix-сокета

int main() {
    char buf[100];
    int fd, rc;

    // создаём сокет в домене AF_UNIX (локальный), тип соединения потоковый (как TCP)
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(-1);
    }

    // инициализация структуры sockaddr_un
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Подключение клиента к серверу по Unix-сокету
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        perror("connect error");
        exit(-1);
    }

    // Читаем данные из stdin и отправляем их на сервер через сокет
    while ((rc = read(STDIN_FILENO, buf, sizeof(buf))) > 0) {
        // Передаём rc байт в сокет (fd)
        if (write(fd, buf, rc) != rc) {
            if (rc > 0) {
                fprintf(stderr, "partial write"); // сокет записал не весь буфер
            } else {
                perror("write error");
                exit(-1);
            }
        }
    }

    return 0;
}
