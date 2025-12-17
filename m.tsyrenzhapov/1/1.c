#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <unistd.h>
#include <string.h>

int main(int argc, char *argv[]){
    int opt;
    
    // если программа запущена без аргументов просто типо ./program
    if (argc == 1){
        printf("Использование: %s [опции]\n", argv[0]);
        printf("Опции:\n");
        printf("  -i  Показать ID пользователя и группы\n");
        printf("  -p  Показать идентификаторы процесса\n");
        printf("  -s  Стать лидером группы процессов\n");
        printf("  -u  Показать значение ulimit\n");
        printf("  -c  Показать размер core-файла\n");
        printf("  -d  Показать текущую директорию\n");
        printf("  -v  Показать переменные среды\n");
        return 0;
    }
    
    while ((opt = getopt(argc, argv, "ispucdv")) != -1){
            switch(opt){
                case 'i': //опция -i
                printf("=== Идентификаторы пользователя и группы ===\n");
                printf("Реальный ID пользователя: %d\n", getuid()); // кто запустил
                printf("Эффективный ID пользователя: %d\n", geteuid()); //от кого (имя) запущено
                printf("Реальный ID группы: %d\n", getgid()); // то же самое для групп
                printf("Эффективный ID группы: %d\n", getegid());
                break;
                
            case 's': //опция -s
                printf("=== Становление лидером группы ===\n");
                if (setpgid(0, 0) == 0){ // сделать процесс лидером 
                    printf("Стал лидером группы процессов. Новый PGID: %d\n", getpgrp()); // айди группы процессов
                } else {
                    perror("Ошибка при установке лидера группы");
                }
                break;
            case 'p': //опция -p
                printf("=== Идентификаторы процесса ===\n");
                printf("ID процесса: %d\n", getpid()); //айди текущего процесса
                printf("ID родительского процесса: %d\n", getppid()); //айди родителя
                printf("ID группы процессов: %d\n", getpgrp()); // айди группы процессов
                break;
                
            case 'u': //опция -u
                printf("=== Ulimit ===\n");
                struct rlimit rlim;
                if (getrlimit(RLIMIT_FSIZE, &rlim) == 0){ //лимит на размер файлов
                    printf("Текущий ulimit: %ld\n", (long)rlim.rlim_cur); //текущее значение лимита
                } else {
                    perror("Ошибка при получении ulimit"); 
                }
                break;
                
            case 'c': //опция -c
                printf("=== Размер core-файла ===\n");
                struct rlimit core_lim;
                if (getrlimit(RLIMIT_CORE, &core_lim) == 0){ //лимит на размер core-файлов
                    printf("Максимальный размер core-файла: %ld байт\n", (long)core_lim.rlim_cur); 
                } else {
                    perror("Ошибка при получении размера core-файла");
                }
                break;
                
            case 'd': //опция -d
                printf("=== Текущая директория ===\n");
                char cwd[1024]; //создаем буфера 1024 байта 
                if (getcwd(cwd, sizeof(cwd)) != NULL){ //здесь путь к директории
                    printf("Текущая директория: %s\n", cwd);
                } else {
                    perror("Ошибка при получении текущей директории");
                }
                break;
                
            case 'v': //опция -v
                printf("=== Переменные среды ===\n");
                extern char **environ;
                for (char **env = environ; *env != NULL; env++) {
                    printf("%s\n", *env);
                }
                break;
                
            case '?': //Неизвестная опция
                printf("Неизвестная опция: %c\n", opt);
                break;
        }
    }
    return 0;
}
// Обновлено: Ср 08 окт 2025 17:53:45 +07
