// Клиент Unix domain socket:
// подключается к серверу и передаёт ему текст

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

// Путь к Unix domain socket, который создаёт сервер
#define SOCKET_PATH "./socket31"
#define BUF 1024

int main() {

    int client_fd;
    struct sockaddr_un socket_addr;

    // Создание Unix domain stream socket
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    memset(&socket_addr, 0, sizeof(socket_addr));
    socket_addr.sun_family = AF_UNIX;
    strncpy(socket_addr.sun_path, SOCKET_PATH, sizeof(socket_addr.sun_path) - 1);

    // Подключение к серверу
    connect(client_fd, (struct sockaddr*)&socket_addr, sizeof(socket_addr));

    int count = 0;

    sleep(2);

    // Передача текста серверу (несколько раз)
    while (count < 20) {
        write(client_fd, "asd", 3);
        count++;
        usleep(10000);
    }

    // Завершение: закрытие соединения
    close(client_fd);
    return 0;
}

