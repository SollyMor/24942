// Напишите две программы, взаимодействующих через Unix domain socket. Первый процесс (сервер) создает сокет и слушает на нем.  При присоединении клиента, сервер получает через соединение текст, состоящий из символов верхнего и нижнего регистров, переводит его в верхний регистр и выводит в свой стандартный поток вывода, аналогично задаче 25. Второй процесс (клиент) устанавливает соединение с сервером и передает ему текст.  После разрыва соединения клиентом, оба процесса завершаются.

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

static const char *socket_path = "./socket30";

static ssize_t robust_read(int fd, void *buf, size_t count) {
    for (;;) {
        ssize_t n = read(fd, buf, count);
        if (n >= 0) return n;
        if (errno == EINTR) continue;
        return -1;
    }
}

static ssize_t robust_write(int fd, const void *buf, size_t count) {
    const char *p = (const char *)buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)n;
        left -= (size_t)n;
    }
    return (ssize_t)count;
}

int main(void) {
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }

    char buffer[8192];
    for (;;) {
        ssize_t n = robust_read(STDIN_FILENO, buffer, sizeof(buffer));
        if (n == 0) break;
        if (n < 0) {
            perror("read");
            close(fd);
            return 1;
        }
        if (robust_write(fd, buffer, (size_t)n) < 0) {
            perror("write");
            close(fd);
            return 1;
        }
    }

    close(fd);
    return 0;
}