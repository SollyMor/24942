#include <stdio.h>     
#include <stdlib.h>     
#include <sys/types.h>  
#include <sys/stat.h>   
#include <unistd.h>     
#include <pwd.h>      
#include <grp.h>        
#include <time.h>       
#include <string.h>     

//ф-я которая возвращает тип файла одним символом
// 'd' - каталог, '-' - обычный файл, '?' - всё остальное.
char file_type_char(mode_t mode){
    //если каталог
    if (S_ISDIR(mode)){
        return 'd';
    }//обычный 
    else if (S_ISREG(mode)){
        return '-';
    } else {
        return '?';
    }
}

//ф-я, которая заполняет строку из 9 символов прав доступа 
//в стиое rwxrwxrwx 
//buf должен иметь размер >= 10, ну то есть 9 символов + \0
void file_perm_string(mode_t mode, char *buf){
    //права владельца - ЧТЕНИЕ
    buf[0] = (mode & S_IRUSR) ? 'r' : '-';
    //права владельца - ЗАПИСЬ
    buf[1] = (mode & S_IWUSR) ? 'w' : '-';
    //права владельца - ИСПОЛНЕНИЕ 
    buf[2] = (mode & S_IXUSR) ? 'x' : '-';

    //права группы - ЧТЕНИЕ
    buf[3] = (mode & S_IRGRP) ? 'r' : '-';
    //права группы - ЗАПИСЬ 
    buf[4] = (mode & S_IWGRP) ? 'w' : '-';
    //права группы - ИСПОЛНЕНИЕ 
    buf[5] = (mode & S_IXGRP) ? 'x' : '-';

    //права остальных - ЧТЕНИЕ 
    buf[6] = (mode & S_IROTH) ? 'r' : '-';
    buf[7] = (mode & S_IWOTH) ? 'w' : '-';
    buf[8] = (mode & S_IXOTH) ? 'x' : '-';

    //завершающий нулевой символ для строки 
    buf[9] = '\0';
}

//ф-я которая по полному пути path возвращает указатель на basename
//то есть только имя файла без директории 
const char *basename_simple(const char *path){
    //ищем последний вход символа / 
    const char *p = strrchr(path, '/');
    //если не нашли, то возвр. исходнуб строку
    if (p == NULL){
        return path;
    }
    //иначе берем все после последнего 
    return p+1;
}

int main(int argc, char *argv[]){
        // Если аргументы есть, перебираем их по одному, начиная с argv[1]
    for (int i = 1; i < argc; i++) {
        const char *path = argv[i];

        // Структура, куда lstat запишет информацию о файле
        struct stat st;
        if (lstat(path, &st) == -1) {
            perror(path);
            continue;
        }

        // Определяем символ типа файла
        char type = file_type_char(st.st_mode);

        // Строка для прав доступа
        char perms[10];
        file_perm_string(st.st_mode, perms);

        // Количество ссылок
        long nlink = (long)st.st_nlink;

        // Узнаём владельца по uid
        struct passwd *pw = getpwuid(st.st_uid);
        const char *user = (pw != NULL) ? pw->pw_name : NULL;

        // Узнаём группу по gid
        struct group *gr = getgrgid(st.st_gid);
        const char *group = (gr != NULL) ? gr->gr_name : NULL;

        // Время модификации
        struct tm *tm_info = localtime(&st.st_mtime);
        char timebuf[64];
        if (tm_info != NULL) {
            // Форматирование даты
            strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm_info);
        } else {
            // Если localtime вернул NULL
            snprintf(timebuf, sizeof(timebuf), "неизвестное время");
        }

        // Флаг: обычный ли файл
        int is_regular = S_ISREG(st.st_mode);

        // Имя файла без пути
        const char *name = basename_simple(path);

        //ниже — форматированный вывод полей в "табличном" виде.
        //сначала тип и права
        printf("%c%s ", type, perms);

        // Число ссылок: ширина 3, выравнивание по правому краю
        printf("%3ld ", nlink);

        // Владелец: если имя найдено, печатаем его, иначе uid числом
        if (user != NULL) {
            //левое выравнивание в поле шириной 8
            printf("%-8s ", user);
        } else {
            //левое выравнивание численного uid
            printf("%-8ld ", (long)st.st_uid);
        }

        // группа - аналогично владельцу
        if (group != NULL) {
            printf("%-8s ", group);
        } else {
            printf("%-8ld ", (long)st.st_gid);
        }

        //размер файла: только если это обычный файл!!!!!!!!!!
        if (is_regular) {
            //поле шириной 8, выравнивание по правому краю
            printf("%8ld ", (long)st.st_size);
        } else {
            //если не обычный файл — просто 9 пробелов (чтобы "столбец размера" был на месте)
            printf("         ");
        }

        //время модификации
        printf("%s ", timebuf);

        //и последнее поле — имя файла (basename)
        printf("%s\n", name);
    }
    return 0;
}
