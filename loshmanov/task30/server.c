// server.c
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/uds_uppercase.sock"

int main(void)
{
    int server_fd, client_fd;
    struct sockaddr_un addr;
    char buf[1024];
    ssize_t nread;

    // Создаём сокет Unix domain
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

    // Подготавливаем адрес и биндимся
    memset(&addr, 0, sizeof(struct sockaddr_un));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

    // Удаляем старый файл сокета, если есть
    unlink(SOCKET_PATH);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }

    // Начинаем слушать
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("Сервер запущен. Ожидание подключения клиента на %s ...\n", SOCKET_PATH);

    // Принимаем одно соединение
    client_fd = accept(server_fd, NULL, NULL);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        unlink(SOCKET_PATH);
        return 1;
    }

    printf("Клиент подключился. Читаю данные...\n");

    // Читаем данные от клиента, переводим в верхний регистр и выводим на stdout
    while ((nread = read(client_fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < nread; ++i) {
            buf[i] = (char)toupper((unsigned char)buf[i]);
        }

        if (write(STDOUT_FILENO, buf, nread) == -1) {
            perror("write to stdout");
            close(client_fd);
            close(server_fd);
            unlink(SOCKET_PATH);
            return 1;
        }
    }

    if (nread == -1) {
        perror("read from client");
    } else {
        printf("\nСоединение с клиентом закрыто, сервер завершает работу.\n");
    }

    close(client_fd);
    close(server_fd);
    unlink(SOCKET_PATH); // удаляем файл сокета

    return 0;
}

