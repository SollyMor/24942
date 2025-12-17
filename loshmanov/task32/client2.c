// Клиент Unix domain socket:
// подключается к серверу и передаёт ему текст

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

// Путь к Unix domain socket, который создаёт сервер
#define SOCKET_PATH "socket32"

static void write_(int fd) {
    // Передача текста серверу (несколько раз)
    for (int i = 0; i < 15; i++) {
        write(fd, "zxc\n", 3);
    }
}

int main(void) {
    int fd;
    struct sockaddr_un address;

    // Создание Unix domain stream socket
    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    // Подключение к серверу
    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    write_(fd);

    // Завершение: закрытие соединения
    close(fd);
    return 0;
}

