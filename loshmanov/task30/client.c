// client.c
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/uds_uppercase.sock"

int main(void)
{
    int sock_fd;
    struct sockaddr_un addr;
    char buf[1024];
    ssize_t nread;

    // Создаём сокет
    sock_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock_fd == -1) {
        perror("socket");
        return 1;
    }

    // Настраиваем адрес и подключаемся к серверу
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    if (connect(sock_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("connect");
        close(sock_fd);
        return 1;
    }

    printf("Подключено к серверу %s\n", SOCKET_PATH);

    // Отправляем 5 строк "asd" в сокет
    for (int i = 0; i < 5; i++) {
        const char *message = "asd\n";
        size_t msg_len = strlen(message);
        ssize_t nwritten = 0;

        while (nwritten < msg_len) {
            ssize_t res = write(sock_fd, message + nwritten, msg_len - nwritten);
            if (res == -1) {
                perror("write to socket");
                close(sock_fd);
                return 1;
            }
            nwritten += res;
        }
        printf("Отправлено: %s", message);
    }

    // Закрываем соединение — сервер увидит EOF и завершится
    close(sock_fd);
    printf("Клиент завершил отправку и закрывает соединение.\n");
    return 0;
}

