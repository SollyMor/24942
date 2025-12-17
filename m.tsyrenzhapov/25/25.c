#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <sys/wait.h>


int main() {
    int pipa[2]; // Массив для дескрипторов канала
    pid_t pid;
    char buffer[1024];
    char vvod[1024];

    pipe(pipa);
    pid = fork();
    
    if (pid < 0){ // Ошибка 
        perror("fork");
        exit(EXIT_FAILURE);
    }


    else if (pid == 0){
        close(pipa[1]); // Закрываем конец для записи

        read(pipa[0], buffer, sizeof(buffer));

        for (int i = 0; buffer[i] != '\0'; i++) {
            buffer[i] = toupper(buffer[i]);
        }

        printf("Новая строка: %s", buffer);
        close(pipa[0]);
    }


    else { // Родительский процесс
        close(pipa[0]); // Закрываем конец для чтения

        printf("Введите строку: ");
        fgets(vvod, sizeof(vvod), stdin);

        write(pipa[1], vvod, strlen(vvod) + 1);
        close(pipa[1]); // Закрываем конец для записи

        wait(NULL);
    }


    return 0;
}