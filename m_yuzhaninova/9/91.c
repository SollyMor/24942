#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main()
{
    // Создаем дочерний процесс 
    pid_t p;
    p = fork();
    
    // Ошибка создания
    if (p == -1) 
    {
        perror("fork");
        return 1;
    }

    // Процесс-потомок
    if (p == 0) 
    {
        execlp("cat", "cat", "data.txt", NULL);
        // Если execlp завершился с ошибкой, то выводим ошибку 
        perror("execlp");
        return 1;
    } 
    // Процесс-родитель
    else 
    {
        printf("\nПроцесс-родитель\n");
        wait(NULL);
    }
    
    return 0;
}
