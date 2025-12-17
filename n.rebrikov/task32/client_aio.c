#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SOCKET_PATH "socket32"

int main(int argc, char *argv[]) {
    int fd;
    struct sockaddr_un address;
    char buffer[256];
    
    if (argc < 2) {
        printf("Usage: %s <client_id>\n", argv[0]);
        return 1;
    }
    
    int client_id = atoi(argv[1]);

    fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("socket");
        return 1;
    }

    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCKET_PATH, sizeof(address.sun_path) - 1);

    if (connect(fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("connect");
        close(fd);
        return 1;
    }
    
    printf("Client %d connected to async server\n", client_id);
    printf("Type messages (Ctrl+D to exit):\n");
    
    // Чтение из stdin и отправка на сервер
    ssize_t n;
    while ((n = read(STDIN_FILENO, buffer, sizeof(buffer) - 1)) > 0) {
        if (write(fd, buffer, n) != n) {
            perror("write");
            break;
        }
    }
    
    close(fd);
    printf("Client %d disconnected\n", client_id);
    return 0;
}
