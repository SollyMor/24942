#include <sys/types.h>
#include <sys/stat.h>
#include <sys/statvfs.h>   // statvfs
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>        // basename

#define TIME_BUF_SIZE 64

// Строим строку вида d rwxrwxrwx (10 символов: тип + 9 прав)
void build_mode_string(mode_t mode, char *buf) {
    if (S_ISDIR(mode))
        buf[0] = 'd';
    else if (S_ISREG(mode))
        buf[0] = '-';
    else
        buf[0] = '?';

    buf[1] = (mode & S_IRUSR) ? 'r' : '-';
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';

    buf[4] = (mode & S_IRGRP) ? 'r' : '-';
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';

    buf[7] = (mode & S_IROTH) ? 'r' : '-';
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';

    buf[10] = '\0';
}

// Возвращает только имя файла (без пути)
const char *get_filename_only(const char *path) {
    static char name_buf[1024];
    strncpy(name_buf, path, sizeof(name_buf) - 1);
    name_buf[sizeof(name_buf) - 1] = '\0';
    return basename(name_buf);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s FILE...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; ++i) {
        const char *path = argv[i];
        struct stat st;

        if (lstat(path, &st) == -1) {
            perror(path);
            continue;
        }

        char mode_str[11];
        build_mode_string(st.st_mode, mode_str);

        long long links = (long long)st.st_nlink;

        struct passwd *pw = getpwuid(st.st_uid);
        const char *user = pw ? pw->pw_name : "?";

        struct group *gr = getgrgid(st.st_gid);
        const char *group = gr ? gr->gr_name : "?";

        // Одно поле: для файлов — размер, для директорий — блок ФС
        char size_or_fs_buf[64];
        size_or_fs_buf[0] = '\0';

        if (S_ISREG(st.st_mode)) {
            snprintf(size_or_fs_buf, sizeof(size_or_fs_buf),
                     "%lld", (long long)st.st_size);
        } else if (S_ISDIR(st.st_mode)) {
            struct statvfs vfs;
            if (statvfs(path, &vfs) == 0) {
                snprintf(size_or_fs_buf, sizeof(size_or_fs_buf),
                         "[FS block size: %lu bytes]",
                         (unsigned long)vfs.f_bsize);
            } else {
                // если statvfs не сработал — оставим пусто (чтобы не портить вывод)
                size_or_fs_buf[0] = '\0';
            }
        }

        // Время модификации
        char time_buf[TIME_BUF_SIZE];
        struct tm *tm_info = localtime(&st.st_mtime);
        if (tm_info)
            strftime(time_buf, sizeof(time_buf),
                     "%Y-%m-%d %H:%M", tm_info);
        else
            strcpy(time_buf, "????????????????");

        const char *fname = get_filename_only(path);

        // Вывод: mode links user group (SIZE/FS) time name
        printf("%-10s %4lld %-10s %-10s %25s %-16s %s\n",
               mode_str,
               links,
               user,
               group,
               size_or_fs_buf,
               time_buf,
               fname);
    }

    return 0;
}

