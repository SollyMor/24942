#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <unistd.h>

void print_file_info(const char *path) {
    struct stat st;
    
    if (lstat(path, &st) != 0) {
        perror(path);
        return;
    }
    
    // File type
    char file_type;
    if (S_ISDIR(st.st_mode)) {
        file_type = 'd';
    } else if (S_ISREG(st.st_mode)) {
        file_type = '-';
    } else {
        file_type = '?';
    }
    
    // Permissions
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
    
    // Number of links
    nlink_t nlinks = st.st_nlink;
    
    // Owner and group names
    struct passwd *pw = getpwuid(st.st_uid);
    struct group *gr = getgrgid(st.st_gid);
    const char *owner = pw ? pw->pw_name : "?";
    const char *group = gr ? gr->gr_name : "?";
    
    // File size (only for regular files)
    // off_t size = S_ISREG(st.st_mode) ? st.st_size : 0;
    off_t size = st.st_size;
    
    // Modification time
    struct tm *tm_info = localtime(&st.st_mtime);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
    
    // Extract filename from path
    const char *filename = strrchr(path, '/');
    if (filename) {
        filename++; // Skip the '/'
    } else {
        filename = path;
    }
    
    // Print formatted output
    if (S_ISREG(st.st_mode)) {
        printf("%c%s %3lu %-8s %-8s %8ld %s %s\n",
               file_type, perms, (unsigned long)nlinks,
               owner, group, (long)size, time_str, filename);
    } else {
        printf("%c%s %3lu %-8s %-8s %8ld %s %s\n",
               file_type, perms, (unsigned long)nlinks,
               owner, group, (long)size, time_str, filename);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2] ...\n", argv[0]);
        return 1;
    }
    
    for (int i = 1; i < argc; i++) {
        print_file_info(argv[i]);
    }
    
    return 0;
}

