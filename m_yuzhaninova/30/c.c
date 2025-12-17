#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>


int main() 
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

    char buf[500];
    long n;

    while ((n = read(0, buf, sizeof(buf))) > 0)
    {
        write(cfd, buf, n);
    }

    close(cfd);
    return 0;
}
