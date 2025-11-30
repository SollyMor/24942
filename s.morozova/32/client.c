#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#define SOCKET_PATH "/tmp/case_converter_socket"
#define BUFFER_SIZE 1024
#define DELAY_SECONDS 1

int client_fd = -1;

void cleanup(int sig) {
    if (client_fd != -1) {
        close(client_fd);
    }
    exit(0);
}

int main(int argc, char *argv[]) {
    struct sockaddr_un server_addr;
    char buffer[BUFFER_SIZE];
    
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <text>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    
    strncpy(buffer, argv[1], BUFFER_SIZE - 1);
    buffer[BUFFER_SIZE - 1] = '\0';
    
    if (buffer[strlen(buffer) - 1] != '\n') {
        strncat(buffer, "\n", BUFFER_SIZE - strlen(buffer) - 1);
    }
    
    signal(SIGINT, cleanup);
    signal(SIGTERM, cleanup);
    
    client_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (client_fd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }
    
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, SOCKET_PATH, sizeof(server_addr.sun_path) - 1);
    
    if (connect(client_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(client_fd);
        exit(EXIT_FAILURE);
    }
    
    printf("Connected to server. Sending text: %s", buffer);
    printf("Will resend every %d second. Press Ctrl+C to stop.\n", DELAY_SECONDS);
    
    while (1) {
        if (write(client_fd, buffer, strlen(buffer)) == -1) {
            perror("write");
            break;
        }
        sleep(DELAY_SECONDS);
    }
    
    printf("Disconnecting from server\n");
    close(client_fd);
    
    return 0;
}