#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>


// Функция для вывода прав доступа
void print_permissions(mode_t mode) 
{
    // Начальные права запрещены
    char perm[11] = "----------";

    // Определяем тип файла
    if (S_ISDIR(mode)) 
    {
        perm[0] = 'd';
    }
    else if (S_ISREG(mode)) 
    {
        perm[0] = '-';
    }
    else 
    {
        perm[0] = '?';
    }

    // Определяем права доступа
    if (mode & S_IRUSR) perm[1] = 'r';
    if (mode & S_IWUSR) perm[2] = 'w';
    if (mode & S_IXUSR) perm[3] = 'x';

    if (mode & S_IRGRP) perm[4] = 'r';
    if (mode & S_IWGRP) perm[5] = 'w';
    if (mode & S_IXGRP) perm[6] = 'x';

    if (mode & S_IROTH) perm[7] = 'r';
    if (mode & S_IWOTH) perm[8] = 'w';
    if (mode & S_IXOTH) perm[9] = 'x';

    // Вывод
    printf("%-10s", perm);
}

// Вывод основной информации о файле
void print_file_info(const char *path) 
{
    // Структура для хранения информации о файле
    struct stat sb;
    // Структура для хранения информации о пользователе
    struct passwd *pw;
    // Структура для хранения информации о группе
    struct group *gr;
    // Структура для хранения информации о времени
    struct tm *tm;
    char date_str[20];

    // Получаем информацию о файле
    if (lstat(path, &sb) == -1) 
    {
        perror(path);
        return;
    }

    // Выводим права доступа
    print_permissions(sb.st_mode);

    // Выводим количество связей
    printf("%*d ", 5, (int)sb.st_nlink);

    // Получаем имя пользователя по UID 
    pw = getpwuid(sb.st_uid);
    // Если получилось, то выводим его
    if (pw) 
    {
        printf("%-*s ", 5, pw->pw_name);
    }
    // Иначе выводим UID
    else 
    {
        printf("%*u ", 5, sb.st_uid);
    }

    // Получаем имя группы
    gr = getgrgid(sb.st_gid);
    // Если получилось, то выводим его
    if (gr)
    {
        printf("%-*s ", 5, gr->gr_name);
    }
    // Иначе выводим GID
    else 
    {
        printf("%*u ", 5, sb.st_gid);
    }

    // Если это файл, то выводим его размер
    if (S_ISREG(sb.st_mode) || S_ISDIR(sb.st_mode))
    {
        printf("%*ld ", 5, sb.st_size);
    } 
    else 
    {
        printf("%*s ", 5, ""); 
    }

    // Получаем текущее время
    tm = localtime(&sb.st_mtime);
    // Красиво его структурируем и выводим
    strftime(date_str, 20, "%b %e %Y", tm);
    printf("%-*s ", 5, date_str);

    // Оставляем только имя файла
    const char *filename = strrchr(path, '/');
    if (filename) 
    {
        filename++;
    }
    else 
    {
        filename = path;
    }
    // Выводим имя файла
    printf("%s\n", filename);
}

int main(int argc, char *argv[]) 
{
    // Проверка на количество файлов
    if (argc < 2) 
    {
        fprintf(stderr, "Должен быть введен хотя бы один файл\n");
        return 1;
    }
    
    // Выводим информацию о каждом файле
    for (int i = 1; i < argc; i++) 
    {
        print_file_info(argv[i]);
    }

    return 0;
}
