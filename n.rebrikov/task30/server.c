#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

char *socket_path = "./socket30";

void cleanup() 
{
    unlink(socket_path);
}

void signal_handler(int sig) 
{
    exit(0);
}

int main() 
{
    char buf[100];
    int fd, cl, rc;

    // Регистрируем обработчик для очистки сокета при завершении
    atexit(cleanup);
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Удаляем старый сокет если существует
    unlink(socket_path);

    if (bind(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) 
    {
        perror("bind error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    if (listen(fd, 5) == -1) 
    {
        perror("listen error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on socket: %s\n", socket_path);

    if ((cl = accept(fd, NULL, NULL)) == -1) 
    {
        perror("accept error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Client connected\n");

    while ((rc = read(cl, buf, sizeof(buf))) > 0) 
    {
        for (int i = 0; i < rc; i++) 
        {
            buf[i] = (char) toupper((unsigned char)buf[i]);
        }
        // Выводим преобразованный текст
        write(STDOUT_FILENO, buf, rc);
    }

    if (rc == -1) 
    {
        perror("read error");
    }

    printf("\nClient disconnected\n");
    close(cl);
    close(fd);
    
    return 0;
}
