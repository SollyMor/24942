#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <ctype.h>
#include <sys/socket.h> // socket(), bind(), listen(), accept(); AF_UNIX, SOCK_STREAM
#include <sys/un.h>     // struct sockaddr_un (адрес Unix domain socket), sun_path
#include <aio.h>        // struct aiocb, aio_read(), aio_error(), aio_return(); SIGEV_SIGNAL
#include <signal.h>     // struct sigaction, sigaction(); siginfo_t; SIGUSR1; SA_SIGINFO

#define BUFFER_SIZE 100   // размер буфера AIO-чтения (за один запрос)
#define BACKLOG 5         // очередь ожидающих подключений (listen backlog)

char *socket_path = "./socket"; // путь к файлу сокета (можно менять, см. примечание ниже)

/* Создаёт AIO-запрос для чтения из сокета */
struct aiocb *create_request(int fd) {
    struct aiocb *req = calloc(1, sizeof(struct aiocb)); // aiocb лучше занулять целиком (calloc)

    req->aio_fildes = fd;               // aiocb: из какого fd читаем (дескриптор клиента)
    req->aio_buf = malloc(BUFFER_SIZE); // aiocb: куда читаем (буфер должен жить до завершения AIO)
    req->aio_nbytes = BUFFER_SIZE;      // aiocb: сколько байт пытаться прочитать

    // sigevent: как уведомлять о завершении aio_read()
    req->aio_sigevent.sigev_notify = SIGEV_SIGNAL;       // уведомление через сигнал
    req->aio_sigevent.sigev_signo = SIGUSR1;             // какой сигнал отправлять процессу
    req->aio_sigevent.sigev_value.sival_ptr = req;       // “payload” для обработчика (передадим указатель)

    return req;
}

/* Обработчик сигнала завершения aio_read */
void aio_handler(int sig, siginfo_t *info, void *ctx) {
    (void)sig;  // sig и ctx в логике не используются
    (void)ctx;

    // SA_SIGINFO даёт доступ к info->si_value (куда AIO кладёт sigev_value)
    struct aiocb *req = (struct aiocb *)info->si_value.sival_ptr;

    // aio_error(): статус последней AIO-операции (0 = OK, EINPROGRESS = ещё выполняется)
    if (aio_error(req) != 0)
        return;

    // aio_return(): сколько реально прочитано (после завершения aio_read)
    ssize_t n = aio_return(req);
    char *buf = (char *)req->aio_buf;

    if (n == 0) {
        // EOF: клиент закрыл соединение (read вернул бы 0)
        close(req->aio_fildes); // закрываем клиентский сокет
        free(buf);              // освобождаем буфер
        free(req);              // освобождаем структуру запроса
        return;
    }

    // Преобразуем в верхний регистр и выводим (по символу, допускаем “смешивание” клиентов)
    for (int i = 0; i < n; i++) {
        putchar(toupper((unsigned char)buf[i])); // (unsigned char) — корректно для toupper()
    }
    fflush(stdout); // чтобы вывод появлялся сразу, а не копился в буфере stdio

    // Перезапускаем асинхронное чтение тем же запросом (циклическая обработка клиента)
    aio_read(req);
}

int main() {
    int fd, cl;

    // socket(AF_UNIX, SOCK_STREAM): локальный потоковый сокет (как TCP, но в файловой системе)
    fd = socket(AF_UNIX, SOCK_STREAM, 0);

    // sockaddr_un: адрес Unix domain socket (sun_path = путь к файлу сокета)
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));                         // зануляем структуру адреса
    addr.sun_family = AF_UNIX;                              // домен Unix-сокетов
    strncpy(addr.sun_path, socket_path, sizeof(addr.sun_path) - 1); // путь (ограничен размером поля)

    unlink(socket_path); // удаляем старый файл сокета, если остался после прошлого запуска

    bind(fd, (struct sockaddr *)&addr, sizeof(addr)); // привязываем fd к пути socket_path
    listen(fd, BACKLOG);                              // переводим в режим ожидания подключений

    // sigaction(): ставим обработчик SIGUSR1, который будет приходить от AIO
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = aio_handler; // обработчик с siginfo_t
    sa.sa_flags = SA_SIGINFO;      // важно: иначе не получим info->si_value
    sigaction(SIGUSR1, &sa, NULL); // регистрируем обработчик

    while (1) {
        // accept(): принимает новое соединение и возвращает новый fd клиента
        cl = accept(fd, NULL, NULL);

        // создаём AIO-запрос для этого клиента и запускаем первое асинхронное чтение
        struct aiocb *req = create_request(cl);
        aio_read(req); // после завершения сработает SIGUSR1 -> aio_handler()
    }
}
