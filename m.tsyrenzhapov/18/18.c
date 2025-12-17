#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

void print_file_info(const char *filename) {
    struct stat stat_buf;
    struct passwd *pwd;
    struct group *grp;
    char mode_str[11];
    char time_str[20];
    char *basename;
    
    if (lstat(filename, &stat_buf) == -1){ //  Получает всю информацию о файле (тип, права, владелец, время и т.д.) 
                                           //  Сохраняет в структуру stat_buf
        perror("lstat");
        return;
    }
    
    mode_str[0] = S_ISDIR(stat_buf.st_mode) ? 'd' : // директория
                  S_ISREG(stat_buf.st_mode) ? '-' : '?'; // файл или другое
    
    // Владелец: чтение запись и выполнение
    mode_str[1] = (stat_buf.st_mode & S_IRUSR) ? 'r' : '-'; 
    mode_str[2] = (stat_buf.st_mode & S_IWUSR) ? 'w' : '-';
    mode_str[3] = (stat_buf.st_mode & S_IXUSR) ? 'x' : '-';
    
    // Группа: чтение запись и выполнение
    mode_str[4] = (stat_buf.st_mode & S_IRGRP) ? 'r' : '-';
    mode_str[5] = (stat_buf.st_mode & S_IWGRP) ? 'w' : '-';
    mode_str[6] = (stat_buf.st_mode & S_IXGRP) ? 'x' : '-';
    
    // Остальные: чтение запись и выполнение
    mode_str[7] = (stat_buf.st_mode & S_IROTH) ? 'r' : '-';
    mode_str[8] = (stat_buf.st_mode & S_IWOTH) ? 'w' : '-';
    mode_str[9] = (stat_buf.st_mode & S_IXOTH) ? 'x' : '-';
    mode_str[10] = '\0';

    pwd = getpwuid(stat_buf.st_uid);  // По ID пользователя получает имя
    grp = getgrgid(stat_buf.st_gid);  // По ID группы получает имя
    
    // Просто формат времени
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", 
             localtime(&stat_buf.st_mtime));
    
    // Находим путь
    basename = strrchr(filename, '/');
    if (basename != NULL) {
        basename++;
    } else {
        basename = (char *)filename;
    }
    
    // Print прав и ссылок
    printf("%s %2ld ", mode_str, (long)stat_buf.st_nlink);
    
    // Имя владельца (мб айди)
    if (pwd != NULL) {
        printf("%-8s ", pwd->pw_name);
    } else {
        printf("%-8d ", (int)stat_buf.st_uid);
    }
    
    // Имя группы (мб айди)
    if (grp != NULL) {
        printf("%-8s ", grp->gr_name);
    } else {
        printf("%-8d ", (int)stat_buf.st_gid);
    }
    
    // Имя размера файлов (мб пустая строка)
    if (S_ISREG(stat_buf.st_mode)) {
        printf("%8ld ", (long)stat_buf.st_size);
    } else {
        printf("%8s ", "");
    }
    
    // Время и название
    printf("%s %s\n", time_str, basename);
}

int main(int argc, char *argv[]) {
    int i;
    
    if (argc < 2) {
        fprintf(stderr, "Введи файл или директорию\n", argv[0]);
        return 1;
    }
    
    for (i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }
    
    return 0;
}