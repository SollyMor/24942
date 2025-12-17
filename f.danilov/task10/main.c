#include <stdio.h> 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// pid_t - тип данных, который определяет идентификатор процесса или группы процессов

int main(int argc, char *argv[]){
    if (argc < 2){
        perror("Ошибка вызова команды");
        return 1;
    }
    
    pid_t pid;
    int rv; // состояние завершения подпроцесса

    switch(pid=fork()) {
        case -1:
            perror("fork failed"); /* произошла ошибка */
            exit(1); /*выход из родительского процесса*/
        case 0:
            printf(" CHILD: Мой PID -- %d\n", getpid());
            execvp(argv[1], &argv[1]); //ищет в path

            // Если мы здесь — значит, execl завершился неудачей
            perror("execvp failed");
            exit(1);
        default:
            printf("PARENT: Мой PID -- %d\n", getpid());

            waitpid(pid, &rv, 0);
            if (WIFEXITED(rv)) {
                printf("PARENT: Потомок завершился нормально. Код завершения: %d\n", WEXITSTATUS(rv));
            } else {
                printf("PARENT: Потомок завершился аварийно.\n");
            }
            
            printf("PARENT: Выход!\n");
        }
}