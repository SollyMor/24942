#include <stdio.h> 
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// pid_t - тип данных, который определяет идентификатор процесса или группы процессов

int main(void){
    pid_t pid;
    int rv; // состояние завершения подпроцесса

    switch(pid=fork()) {
        case -1:
            perror("fork failed"); /* произошла ошибка */
            exit(1); /*выход из родительского процесса*/
        case 0:
            printf(" CHILD: Это процесс-потомок!\n");
            printf(" CHILD: Мой PID -- %d\n", getpid());
            execlp("cat", "cat", "in.txt", (char *)NULL); //ищет в path

            // Если мы здесь — значит, execl завершился неудачей
            perror("execl failed");
            exit(1);
        default:
            printf("PARENT: Это процесс-родитель!\n");
            printf("PARENT: Мой PID -- %d\n", getpid());

            printf("PARENT: Я жду, пока потомок не вызовет exit()...\n");
            waitpid(pid, &rv, 0);

            if (WIFEXITED(rv)) {
                printf("PARENT: Последняя строка, распечатанная родителем: {PARENT: Это процесс-родитель!}\n");
                printf("PARENT: Потомок завершился нормально.\n");
            } else {
                printf("PARENT: Потомок завершился аварийно.\n");
            }
            
            printf("PARENT: Выход!\n");
        }
}