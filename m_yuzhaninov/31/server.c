#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <ctype.h>
#include <sys/select.h>

#define SOCKET "./mysocket"

int main() 
{
    // Дескриптор для сервера
    int server_fd;

    // Создаем сокет
    server_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (server_fd == -1) 
    {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    // Структура для записи адреса сервера
    struct sockaddr_un server_addr = {0};
    // Говорим что используется семейство UNIX сокетов
    server_addr.sun_family = AF_UNIX;
    // Устанавливаем путь к файлу сокета 
    strncpy(server_addr.sun_path, SOCKET, sizeof(server_addr.sun_path) - 1);

    // Удаляем старый сокет, если он существует
    unlink(SOCKET);
    // Привязываем сокет к адресу
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) 
    {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    
    // Начинаем слушать соединения
    if (listen(server_fd, 5) == -1) 
    {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }
    printf("Сервер запущен и слушает\n");
    
    // Дескриптор клиента
    int client_fd;
    // Структура адреса клиента
    struct sockaddr_un client_addr = {0};
    socklen_t client_len = sizeof(client_addr);

    // Массив для клиентов
    int clients[5];
    for (int i = 0; i < 5; i++)
    {
        clients[i] = -1;
    }

    // Множество файловых дескрипторов для select
    fd_set readfds;

    while (1)
    {
        // Чистим набор дескрипторов
        FD_ZERO(&readfds);
        // Говорим, что нужно следить за серверным дескриптором
        FD_SET(server_fd, &readfds);
        // Нужен максимальный файловый дескриптор
        int maxfd = server_fd;

        // Говорим за какими клиентами нужно следить
        for (int i = 0; i < 5; i++) 
        {
            if (clients[i] != -1) 
            {
                FD_SET(clients[i], &readfds);
                if (clients[i] > maxfd)
                {
                    maxfd = clients[i];
                }
            }
        }

        // Ждём активности
        int activity = select(maxfd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) 
        {
            perror("select");
            break;
        }        

        // Если активен сокет сервера, то пытается подключится клиент
        if (FD_ISSET(server_fd, &readfds))
        {
            // Подключаем клиента
            client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
            if (client_fd == -1) 
            {
                perror("accept");
                close(server_fd);
                exit(EXIT_FAILURE);
            }

            // Добавляем клиента в массив
            int added = 0;
            for (int i = 0; i < 5; i++) 
            {
                // Если есть место
                if (clients[i] == -1) 
                {
                    // Добавляем дескриптор клиента в массив
                    clients[i] = client_fd;
                    added = 1;
                    break;
                }
            }

            // Если не получилось добавить значит нет места
            if (!added) 
            {
                printf("Максимум клиентов! Отклонение...\n");
                close(client_fd);
            }
        }

        // Проверяем готов ли какой нибудь клиент к чтению
        for (int i = 0; i < 5; i++) 
        {
            int fd = clients[i];
            // Если клиент активен, то читаем его
            if (fd != -1 && FD_ISSET(fd, &readfds)) 
            {
                char symb; 
                long bytes = read(fd, &symb, 1);
                // Если клиент закончил, то отключаем его
                if (bytes <= 0) 
                {
                    close(fd);
                    clients[i] = -1;
                    continue;
                }

                symb = toupper(symb);

                // Печатаем результат
                printf("[C%d] %c\n", fd, symb);
                fflush(stdout);
            }
        }
    }
    
    // Закрываем соединения
    close(server_fd);
    
    // Удаляем файл сокета
    unlink(SOCKET);
    
    return 0;
}