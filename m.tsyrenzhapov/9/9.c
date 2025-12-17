#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// int main() {
//     pid_t pid = fork();

//     if (pid == -1) {
//         perror("fork failed");
//         exit(EXIT_FAILURE);
//     } else if (pid == 0) {
//         // Подпроцесс: выполняем cat для длинного файла
//         execlp("cat", "cat", "longfile.txt", (char *)NULL);
//     } else {
//         // Родительский процесс: выводим текст
//         printf("Родительский процесс: текст выведен.\n");
//     }

//     return 0;
// }


int main() {
    pid_t pid = fork();

    if (pid == 0) {
        execlp("cat", "cat", "longfile.txt", (char *)NULL);
    } 
    else if(pid >0){
        int status;
        waitpid(pid, &status, 0);

        printf("Родительский процесс: текст выведен\n");
    }

    return 0;
}
