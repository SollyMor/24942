#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

int main() {
    int server_socket;
    struct sockaddr_un server_addr;
    char buffer_in[1024];
    char buffer_out[1024];
    // Создаем сокет
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sun_family = AF_UNIX;
    strcpy(server_addr.sun_path, "task31_socket");

    // подключаемся к серверу
    if (connect(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    fgets(buffer_in, sizeof(buffer_in), stdin);

    write(server_socket, buffer_in, strlen(buffer_in));
    close(server_socket);

    return 0;
}