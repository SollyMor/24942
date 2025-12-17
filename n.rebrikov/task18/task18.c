#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>

void print_permissions(mode_t mode) 
{
    // Тип файла  
    if (S_ISDIR(mode)) printf("d");
    else if (S_ISREG(mode)) printf("-");
    else if (S_ISLNK(mode)) printf("l");
    else if (S_ISCHR(mode)) printf("c");
    else if (S_ISBLK(mode)) printf("b");
    else if (S_ISFIFO(mode)) printf("p");
    else if (S_ISSOCK(mode)) printf("s");
    else printf("?");

    // Права доступа владельца
    printf("%c", (mode & S_IRUSR) ? 'r' : '-');
    printf("%c", (mode & S_IWUSR) ? 'w' : '-');
    printf("%c", (mode & S_IXUSR) ? 'x' : '-');

    // Права доступа группы
    printf("%c", (mode & S_IRGRP) ? 'r' : '-');
    printf("%c", (mode & S_IWGRP) ? 'w' : '-');
    printf("%c", (mode & S_IXGRP) ? 'x' : '-');

    // Права доступа остальных
    printf("%c", (mode & S_IROTH) ? 'r' : '-');
    printf("%c", (mode & S_IWOTH) ? 'w' : '-');
    printf("%c", (mode & S_IXOTH) ? 'x' : '-');
}



void print_file_info(const char *path) 
{
    struct stat st;
    
    if (lstat(path, &st) == -1) 
    {
        fprintf(stderr, "ls: cannot access '%s': ", path);
        perror("");
        return;
    }
    
    // Права доступа и тип файла
    print_permissions(st.st_mode);
    
    // Количество связей
    printf(" %3ld", (long)st.st_nlink);
    
    // Владелец и группа
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    
    printf(" %-8s", pw ? pw->pw_name : "?");
    printf(" %-8s", gr ? gr->gr_name : "?");
    
    // Размер для разных типов файлов
    if (S_ISREG(st.st_mode)) 
    {
    // Обычный файл - показываем его размер
        printf(" %8ld", (long)st.st_size);
    } 
    else if (S_ISDIR(st.st_mode))
    {
    // Директория - показываем размер метаданных (как в ls)
        printf(" %8ld", (long)st.st_size);
    }
    else 
    {
    // Для других типов (символические ссылки и т.д.)
        printf(" %8s", "");
    }
    
    // Дата модификации
    char time_buf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    strftime(time_buf, sizeof(time_buf), "%b %d %H:%M", tm_info);
    printf(" %s", time_buf);
    
    // Имя файла
    const char *filename = strrchr(path, '/');
    if (filename && *(filename + 1) != '\0') 
    {
        filename++;
    } 
    else 
    {
        filename = path;
    }
    
    // Для символических ссылок показываем цель
    if (S_ISLNK(st.st_mode)) 
    {
        char link_buf[1024];
        ssize_t len = readlink(path, link_buf, sizeof(link_buf) - 1);
        if (len != -1) 
        {
            link_buf[len] = '\0';
            printf(" %s -> %s\n", filename, link_buf);
        } 
        else 
        {
            printf(" %s\n", filename);
        }
    } 
    else 
    {
        printf(" %s\n", filename);
    }
}

int main(int argc, char *argv[]) 
{
    if (argc < 2) 
    {
        // Если аргументов нет - используем текущую директорию
        print_file_info(".");
    } 
    else 
    {
        for (int i = 1; i < argc; i++) 
        {
            print_file_info(argv[i]);
        }
    }
    
    return 0;
}
