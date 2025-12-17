#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>

#define MAX_BUFFER_SIZE 1024

int main(void){
    int streamPARENT_WR[2];  // parent → child
    int streamCHILD_WR[2];   // child → parent

    if (pipe(streamPARENT_WR) == -1 || pipe(streamCHILD_WR) == -1){
        perror("Pipe stream error");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork failed");
        exit(1);
    }

    if (pid == 0) {
        // закрываем ненужные стороны
        close(streamPARENT_WR[1]);
        close(streamCHILD_WR[0]);

        char buf[MAX_BUFFER_SIZE];
        int n = read(streamPARENT_WR[0], buf, sizeof(buf));
        if (n == -1){
            perror("read_out error");
            exit(EXIT_FAILURE);
        }
        close(streamPARENT_WR[0]);

        // обработка только фактически прочитанных данных
        for (int i = 0; i < n; i++){
            buf[i] = tolower(buf[i]);
        }
        // отправка результата родителю
        write(streamCHILD_WR[1], buf, n);
        close(streamCHILD_WR[1]);

        exit(EXIT_SUCCESS);

    } 
    else {

        // закрываем ненужные стороны
        close(streamPARENT_WR[0]);
        close(streamCHILD_WR[1]);

        char buffer[MAX_BUFFER_SIZE];
        scanf("%1023s", buffer);
        int len = strlen(buffer);

        // отправка реальной длины строки
        write(streamPARENT_WR[1], buffer, len);
        close(streamPARENT_WR[1]);

        // читаем ответ из child
        int n = read(streamCHILD_WR[0], buffer, sizeof(buffer));
        if (n == -1){
            perror("read_in error");
            exit(EXIT_FAILURE);
        }
        close(streamCHILD_WR[0]);

        buffer[n] = '\0'; // завершаем строку

        printf("%s\n", buffer);
        printf("Конец работы\n");
    }
}
