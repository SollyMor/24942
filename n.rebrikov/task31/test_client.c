#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

char *socket_path = "./socket31";

int main(int argc, char *argv[]) 
{
    char buf[256];
    int fd;
    
    // Требуем ID клиента
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <client_id>\n", argv[0]);
        fprintf(stderr, "Example: %s 1\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    int client_id = atoi(argv[1]);

    // Создание сокета
    if ((fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) 
    {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    // Настройка адреса
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1);

    // Подключение к серверу
    if (connect(fd, (struct sockaddr *) &addr, sizeof(addr)) == -1) 
    {
        perror("connect error");
        close(fd);
        exit(EXIT_FAILURE);
    }

    printf("Test Client %d connected\n", client_id);

    // Сообщения для смешения
    const char *patterns[] = {
        "AAA", "BBB", "CCC", "DDD", "EEE", 
        "FFF", "GGG", "HHH", "III", "JJJ"
    };
    
    // Случайная задержка для каждого клиента
    srand(time(NULL) + client_id);
    int base_delay = 30000 + (rand() % 50000); // 30-80ms
    
    // Отправляем 15 сообщений
    for (int i = 0; i < 15; i++) {
        // Формируем сообщение
        int pattern_idx = (i + client_id) % 10;
        snprintf(buf, sizeof(buf), "[Client %d] Message %02d: %s%d%d%d\n", 
                client_id, i+1, 
                patterns[pattern_idx], client_id, client_id, client_id);
        
        // Отправляем
        if (write(fd, buf, strlen(buf)) != strlen(buf)) {
            perror("write error");
            break;
        }
        
        printf("Client %d sent: %s", client_id, buf);
        
        // Случайная задержка для создания смешения
        usleep(base_delay + (rand() % 20000));
    }

    // Завершающее сообщение
    snprintf(buf, sizeof(buf), "[Client %d] FINISHED\n", client_id);
    write(fd, buf, strlen(buf));
    
    // Закрытие
    shutdown(fd, SHUT_WR);
    close(fd);
    
    printf("Test Client %d disconnected\n", client_id);
    return 0;
}
