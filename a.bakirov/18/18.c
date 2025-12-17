#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

void print_mode(mode_t mode) {
    char type;

    if (S_ISDIR(mode))
        type = 'd';
    else if (S_ISREG(mode))
        type = '-';
    else
        type = '?';

    putchar(type);

    putchar((mode & S_IRUSR) ? 'r' : '-');
    putchar((mode & S_IWUSR) ? 'w' : '-');
    putchar((mode & S_IXUSR) ? 'x' : '-');

    putchar((mode & S_IRGRP) ? 'r' : '-');
    putchar((mode & S_IWGRP) ? 'w' : '-');
    putchar((mode & S_IXGRP) ? 'x' : '-');

    putchar((mode & S_IROTH) ? 'r' : '-');
    putchar((mode & S_IWOTH) ? 'w' : '-');
    putchar((mode & S_IXOTH) ? 'x' : '-');
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s file...\n", argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        struct stat st;
        if (stat(argv[i], &st) == -1) {
            perror(argv[i]);
            continue;
        }

        print_mode(st.st_mode);

        printf(" %2lu", (unsigned long)st.st_nlink);

        struct passwd *pw = getpwuid(st.st_uid);
        struct group  *gr = getgrgid(st.st_gid);

        printf(" %-8s %-8s",
               pw ? pw->pw_name : "?",
               gr ? gr->gr_name : "?");

        if (S_ISREG(st.st_mode))
            printf(" %10ld", (long)st.st_size);
        else
            printf(" %10ld", (long)st.st_size);

        char *t = ctime(&st.st_mtime);
        if (t) {
            t[strcspn(t, "\n")] = '\0';
            printf(" %s", t);
        }

        const char *name = argv[i];
        const char *slash = strrchr(name, '/');
        if (slash)
            name = slash + 1;

        printf(" %s\n", name);
    }

    return 0;
}
