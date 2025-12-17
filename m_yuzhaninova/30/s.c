#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>


int main() 
{
    int sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd < 0) 
    {
	perror("socket");
        exit(1);
    }

    struct sockaddr_un addr = {0};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "./socket", sizeof(addr.sun_path) - 1);

    unlink("./socket");
    if (bind(sfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) 
    {
	perror("bind");
        exit(1);
    }
    if (listen(sfd, 1) < 0)
    {
	perror("listen");
        exit(1);
    }
    printf("Сервер запущен\n");
    
    struct sockaddr_un caddr = {0};
    socklen_t clen = sizeof(caddr);
    int cfd = accept(sfd, (struct sockaddr*)&caddr, &clen);
    if (cfd < 0) 
    {
        exit(1);
    }

    char buf[500];
    long n;

    while ((n = read(cfd, buf, sizeof(buf))) > 0) 
    {
        for (int i = 0; i < n; i++)
        {
            buf[i] = toupper(buf[i]);
        }
        write(1, buf, n);
    }

    close(cfd);
    close(sfd);
    unlink("./socket");
    return 0;
}
