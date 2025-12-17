#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>

int main() {

    unlink("task30_socket");
    
    int server_socket, client_socket;
    // адрес локкального сокета
    struct sockaddr_un server_addr, client_addr;
    char buffer[1024];
    
    // создание стандартного сокета(возвращает дескриптор файла)
    server_socket = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_socket  == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sun_family = AF_UNIX;
    // создаст файл в текущей директории чтобы через него связываться
    strcpy(server_addr.sun_path, "task30_socket");
    int slen = sizeof(server_addr);

    // связываем сокет
    bind(server_socket, (struct sockaddr *)&server_addr, slen);
  
    // cлушаем подключение, кол-во очередей 1
    listen(server_socket, 1);
    printf("Server listening on task30_socket\n");

    // принимаем подключение
    int clen = sizeof(client_addr);
    client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &clen);

    // читаем данные
    ssize_t bytes_read = 0;
    bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    buffer[bytes_read] = '\0';

    for (size_t i = 0; i < bytes_read; i++) {
        buffer[i] = toupper(buffer[i]);
    }

    write(STDOUT_FILENO, buffer, bytes_read);

    close(client_socket);
    close(server_socket);
    unlink("task30_socket");

    return 0;
}