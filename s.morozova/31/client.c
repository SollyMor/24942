#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>

#define SOCKET_PATH "/tmp/task31_socket"
#define BUFFER_SIZE 1024

static void send_with_retry(int fd, const char *message) {
    size_t to_write = strlen(message);
    size_t written_total = 0;

    while (written_total < to_write) {
        ssize_t written = write(fd, message + written_total, to_write - written_total);
        if (written == -1) {
            perror("write");
            close(fd);
            exit(EXIT_FAILURE);
        }
        written_total += (size_t)written;
    }
}

int main(int argc, char *argv[]) {
    int client_fd;
    struct sockaddr_un server_addr;
    char message[BUFFER_SIZE] = "message\n";
    
    // Если указан аргумент, используем его как сообщение
    if (argc > 1) {
        snprintf(message, sizeof(message), "%s\n", argv[1]);
    }
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }

    // Отправляем одно сообщение
    send_with_retry(client_fd, message);
    
    close(client_fd);
    
    return EXIT_SUCCESS;
}