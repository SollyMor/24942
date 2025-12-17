#include <stdio.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>

char* getbase(char* path) {
    char* base = strrchr(path, '/');
    return base ? base + 1 : path;
}

char gettype(mode_t mode) {
    if (S_ISDIR(mode)) return 'd';
    if (S_ISREG(mode)) return '-';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '?';
}

void format_date_ls(time_t mtime, char* buffer, size_t size) {
    struct tm* tm_info = localtime(&mtime);
    time_t now = time(NULL);
    struct tm* now_info = localtime(&now);
    
    if (now_info->tm_year - tm_info->tm_year > 0 || 
        (now_info->tm_year == tm_info->tm_year && now_info->tm_mon - tm_info->tm_mon > 6)) {
        strftime(buffer, size, "%b %_d  %Y", tm_info);  // %_d для убирания ведущего нуля
    } else {
        strftime(buffer, size, "%b %_d %H:%M", tm_info);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file1> [file2 ...]\n", argv[0]);
        return 1;
    }
    
    setlocale(LC_ALL, "");
    
    for (int i = 1; i < argc; i++) {
        struct stat sb;
        
        if (lstat(argv[i], &sb) == -1) {
            printf("Cannot access '%s'\n", argv[i]);
            continue;
        }
        
        printf("%c", gettype(sb.st_mode));
        printf("%c%c%c", sb.st_mode & S_IRUSR ? 'r' : '-',
                         sb.st_mode & S_IWUSR ? 'w' : '-',
                         sb.st_mode & S_IXUSR ? 'x' : '-');
        printf("%c%c%c", sb.st_mode & S_IRGRP ? 'r' : '-',
                         sb.st_mode & S_IWGRP ? 'w' : '-',
                         sb.st_mode & S_IXGRP ? 'x' : '-');
        printf("%c%c%c", sb.st_mode & S_IROTH ? 'r' : '-',
                         sb.st_mode & S_IWOTH ? 'w' : '-',
                         sb.st_mode & S_IXOTH ? 'x' : '-');
        
        printf(" %ld", (long)sb.st_nlink);
        
        struct passwd* pwd = getpwuid(sb.st_uid);
        struct group* grp = getgrgid(sb.st_gid);
        printf(" %s %s", 
               pwd ? pwd->pw_name : "?", 
               grp ? grp->gr_name : "?");
        
        printf(" %ld", (long)sb.st_size);
        
        char date_buf[32];
        format_date_ls(sb.st_mtime, date_buf, sizeof(date_buf));
        printf(" %s", date_buf);
        
        printf(" %s\n", getbase(argv[i]));
    }
    
    return 0;
}