#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> //pid_t

void print_file_info(const char *path) {
    struct stat sb;
    struct passwd *pwd;
    struct group *grp;
    char time[20];
    char stat[12];
    const char *basename = strrchr(path, '/');
    if (basename == NULL) {
        basename = path;
    } else {
        basename++;
    }

    if (lstat(path, &sb) == -1) {
        perror("No such file or directory\n");
        return;
    }

    // Тип файла
    if (S_ISDIR(sb.st_mode)) {
        stat[0] = 'd';
    } else if (S_ISREG(sb.st_mode)) {
        stat[0] = '-';
    } else {
        stat[0] = '?';
    }

    // Права доступа
    //sb.st_mode - битовая маска
    stat[1] = (sb.st_mode & S_IRUSR) ? 'r' : '-';
    stat[2] = (sb.st_mode & S_IWUSR) ? 'w' : '-';
    stat[3] = (sb.st_mode & S_IXUSR) ? 'x' : '-';
    stat[4] = (sb.st_mode & S_IRGRP) ? 'r' : '-';
    stat[5] = (sb.st_mode & S_IWGRP) ? 'w' : '-';
    stat[6] = (sb.st_mode & S_IXGRP) ? 'x' : '-';
    stat[7] = (sb.st_mode & S_IROTH) ? 'r' : '-';
    stat[8] = (sb.st_mode & S_IWOTH) ? 'w' : '-';
    stat[9] = (sb.st_mode & S_IXOTH) ? 'x' : '-';
    stat[10] = '.';
    stat[11] = '\0';
    //printf("\n%d\n%d", sb.st_mode, S_IRUSR);
    
    pwd = getpwuid(sb.st_uid);
    grp = getgrgid(sb.st_gid);   
    char pwd_name[32];
    if (pwd != NULL) {
        strcpy(pwd_name, pwd->pw_name);
    } else {
        sprintf(pwd_name, "%ld", (long)sb.st_uid);
    }

    char grp_name[32];
    if (grp != NULL) {
        strcpy(grp_name, grp->gr_name);
    } else {
        sprintf(grp_name, "%ld", (long)sb.st_gid);
    }

    strftime(time, sizeof(time), "%b %d %H:%M", localtime(&sb.st_mtime));

    printf("%s %ld %s %s %lld %s %s\n", stat, (long)sb.st_nlink, pwd_name, grp_name, (long long)sb.st_size, time, basename);
}

int main(int argc, char *argv[]) {
    // Обработка текущего каталога
    if (argc == 1){
        char *curr_dir = ".";
        print_file_info(curr_dir);
    } 
    else {
        for (int i = 1; i < argc; i++) {
            printf("%s:\n", argv[i]);
            printf("Мой ls -ld: ");
            print_file_info(argv[i]);
            
            printf("Системный:  ");
            fflush(stdout);
            
            char cmd[256];
            snprintf(cmd, sizeof(cmd), "ls -ld '%s'", argv[i]);
            int result = system(cmd);
            if (result == -1) {
                perror("system() failed");
            }
            printf("\n");
        }
    }
    return 0;
}