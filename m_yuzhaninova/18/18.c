#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

void show_rights(mode_t file_mode) 
{
    char rights[11] = "?---------";

    if (S_ISDIR(file_mode)) 
    {
        rights[0] = 'd';
    }
    else if (S_ISREG(file_mode))
    {
         rights[0] = '-';
    }

    if (file_mode & S_IRUSR) rights[1] = 'r';
    if (file_mode & S_IWUSR) rights[2] = 'w';
    if (file_mode & S_IXUSR) rights[3] = 'x';
    if (file_mode & S_IRGRP) rights[4] = 'r';
    if (file_mode & S_IWGRP) rights[5] = 'w';
    if (file_mode & S_IXGRP) rights[6] = 'x';
    if (file_mode & S_IROTH) rights[7] = 'r';
    if (file_mode & S_IWOTH) rights[8] = 'w';
    if (file_mode & S_IXOTH) rights[9] = 'x';

    printf("%s", rights);
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        fprintf(stderr, "\nНе хватает аргументов\n");
        return 1;
    }
    
    for (int i = 1; i < argc; i++) 
    {
        // Получаем информацию о файле
        struct stat info;
        if (lstat(argv[i], &info) == -1) 
        {
            perror("lstat");
            continue;
        }

        // Права
        show_rights(info.st_mode);
        printf(" %3ld", info.st_nlink);

        // Имя пользователя
        struct passwd *user = getpwuid(info.st_uid);
        if (user)
        {
            printf(" %-8s", user->pw_name);
        }
        else 
        {
            printf(" %-8d", info.st_uid);
        }

        // Имя группы
        struct group *group = getgrgid(info.st_gid);
        if (group)
        {
            printf(" %-8s", group->gr_name);
        }
        else 
        {
            printf(" %-8d", info.st_gid);
        }

        // Размер
        if (S_ISREG(info.st_mode) || S_ISDIR(info.st_mode))
        {
            printf(" %8ld", info.st_size);
        }
        else
        { 
            printf(" %8s", "");
        }

        // Время
        struct tm *time = localtime(&info.st_mtime);
        char string[20];
        strftime(string, 20, "%b %e %Y", time);
        printf(" %15s", string);

        // Имя файла
        const char *file_name = strrchr(argv[i], '/');
        printf(" %s\n", file_name ? file_name + 1 : argv[i]);
    }

    return 0;
}