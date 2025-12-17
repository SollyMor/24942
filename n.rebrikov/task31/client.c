#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>

char *socket_path = "./socket31";

int main(int argc, char *argv[]) 
{
    char buf[256];
    int fd, rc;
    int client_id = 1;
    
    // Можно передать ID клиента как аргумент
    if (argc > 1) {
        client_id = atoi(argv[1]);
    }

    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) 
    {
        perror("connect error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Client %d connected to server\n", client_id);

    // Автоматические сообщения для демонстрации смешения
    const char *messages[] = 
    {
        "Hello from client %d\n",
        "AAAAAAAAAAAAAAAAAAAAAAAAA\n", 
        "BBBBBBBBBBBBBBBBBB\n",
        "CCCCCCCCCCCCCCCCC\n",
        "Goodbye from client %d\n"
    };
    
    int num_messages = sizeof(messages) / sizeof(messages[0]);
    
    for (int i = 0; i < num_messages; i++) 
    {
        // Формируем сообщение с ID клиента
        snprintf(buf, sizeof(buf), messages[i], client_id);
        
        // Отправляем сообщение
        rc = write(fd, buf, strlen(buf));
        if (rc != strlen(buf)) 
        {
            perror("write error");
            break;
        }
        
        printf("Client %d sent: %s", client_id, buf);
        
        // Задержка между сообщениями (разная для разных клиентов)
        usleep(3000000 * client_id); // 50ms * client_id
    }

    // Корректное закрытие
    shutdown(fd, SHUT_WR);
    close(fd);
    
    printf("Client %d disconnected\n", client_id);
    return 0;
}
