#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define SOCKET_PATH "socket32"

int main(int argc, char *argv[]) {
    int fd;
    struct sockaddr_un address;
    
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
    
    printf("Client %d connected to server\n", client_id);
    
    // Отправляем БОЛЬШИЕ сообщения по частям, чтобы они могли смешаться
    // в буфере TCP/socket
    
    if (client_id == 1) {
        // Клиент 1 отправляет много маленьких сообщений быстро
        for (int i = 0; i < 20; i++) {
            char buf[50];
            // Сообщения БЕЗ \n в середине, чтобы они склеивались
            snprintf(buf, sizeof(buf), "C1_MSG%02d_", i + 1);
            write(fd, buf, strlen(buf));
            printf("C1 sent part: %s\n", buf);
            usleep(1000); // ОЧЕНЬ маленькая задержка - 1ms
        }
        // Только в конце \n
        write(fd, "\n", 1);
        
    } else if (client_id == 2) {
        // Клиент 2 тоже отправляет много маленьких сообщений
        for (int i = 0; i < 20; i++) {
            char buf[50];
            snprintf(buf, sizeof(buf), "C2_MSG%02d_", i + 1);
            write(fd, buf, strlen(buf));
            printf("C2 sent part: %s\n", buf);
            usleep(1000); // ОЧЕНЬ маленькая задержка - 1ms
        }
        write(fd, "\n", 1);
    }
    
    write(fd, "END\n", 4);
    
    usleep(100000);
    close(fd);
    printf("Client %d disconnected\n", client_id);
    return 0;
}
