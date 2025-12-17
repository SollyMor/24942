#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

long long get_dir_size(const char *path) {
    long long total = 0;

    DIR *dir = opendir(path);
    if (!dir) return 0;

    struct dirent *entry;
    struct stat st;
    char fullpath[4096];

    while ((entry = readdir(dir)) != NULL) {

        // Пропускаем . и ..
        if (strcmp(entry->d_name, ".") == 0 ||
            strcmp(entry->d_name, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);

        if (lstat(fullpath, &st) == -1)
            continue;

        if (S_ISDIR(st.st_mode)) {
            total += get_dir_size(fullpath);
        } else if (S_ISREG(st.st_mode)) {
            total += st.st_size;
        }
    }

    closedir(dir);
    return total;
}

static void print_separator() {
    printf("+---+-----------+------------+------------+------------+-----------------+------------------+\n");
}

static void print_header() {
    print_separator();
    printf("| T | PERMS     | USER       | GROUP      |       SIZE | MTIME           | NAME             |\n");
    print_separator();
}

static void print_file_info(const char *path) {
    struct stat st;

    if (lstat(path, &st) == -1) {
        perror(path);
        return;
    }

    // TYPE
    char type;
    if (S_ISDIR(st.st_mode)) type = 'd';
    else if (S_ISREG(st.st_mode)) type = '-';
    else type = '?';

    // PERMS
    char perms[10];
    perms[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    perms[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    perms[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
    perms[3] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    perms[4] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    perms[5] = (st.st_mode & S_IXGRP) ? 'x' : '-';
    perms[6] = (st.st_mode & S_IROTH) ? 'r' : '-';
    perms[7] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    perms[8] = (st.st_mode & S_IXOTH) ? 'x' : '-';
    perms[9] = '\0';

    // USER
    struct passwd *pw = getpwuid(st.st_uid);
    const char *user = pw ? pw->pw_name : "?";

    // GROUP
    struct group *gr = getgrgid(st.st_gid);
    const char *group = gr ? gr->gr_name : "?";

    // SIZE
    char sizebuf[16];
    if (S_ISREG(st.st_mode)) {
        snprintf(sizebuf, sizeof(sizebuf), "%ld", (long)st.st_size);
    } else if (S_ISDIR(st.st_mode)) {
        long long total = get_dir_size(path);
        snprintf(sizebuf, sizeof(sizebuf), "%ld", total);
    } else {
        sizebuf[0] = '\0';
    }

    // MTIME
    char timebuf[64];
    struct tm *tm_info = localtime(&st.st_mtime);
    if (tm_info)
        strftime(timebuf, sizeof(timebuf), "%b %e %H:%M", tm_info);
    else
        strcpy(timebuf, "unknown");

    // NAME
    const char *name = strrchr(path, '/');
    name = name ? name + 1 : path;


    printf("| %-1.1c | %-9.9s | %-10.10s | %-10.10s | %10.10s | %-15.15s | %-16.16s |\n",
           type, perms, user, group, sizebuf, timebuf, name);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    print_header();

    for (int i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }

    print_separator();

    return 0;
}
