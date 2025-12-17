#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <time.h>


int main(int argc, char *argv[]) 
{
    int cfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (cfd < 0) 
    {
        exit(1);
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "./socket", sizeof(addr.sun_path) - 1);

    if (connect(cfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
        exit(1);
    }
    printf("Соединение установлено\n");

    char *t = "rrrrrrrrrrr";
    if (argc == 2)
    {
        t = argv[1];
    }

    time_t start = time(NULL);
    time_t new_time;

    while (difftime(new_time, start) < 5.0)
    {
        write(cfd, t, strlen(t));
        new_time = time(NULL);
    }

    char buf[500];
    long n;

    while ((n = read(0, buf, sizeof(buf))) > 0)
    {
        write(cfd, buf, n);
    }

    close(cfd);
    return 0;
}
