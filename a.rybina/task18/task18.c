/*
 * Программа - аналог команды ls -ld
 * Выводит подробную информацию о файлах и каталогах
 * 
 * Формат вывода:
 * - Тип файла (d для каталога, - для обычного файла)
 * - Права доступа (rwx для владельца, группы и остальных)
 * - Количество ссылок
 * - Имя владельца и группы
 * - Размер файла (только для обычных файлов)
 * - Дата модификации
 * - Имя файла
 */

#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

int main(int argc, char* argv[]) {
    // Check number of arguments
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [filename ...]\n to get file information", argv[0]);
        return 1;
    }

    // Process each passed file
    for (int i = 1; i < argc; i++) {
        char* filename = argv[i];
        struct stat file_stat;
        
        // Get file information
        if (stat(filename, &file_stat) == -1) {
            fprintf(stderr, "Error getting information about file '%s': ", filename);
            perror("");
            continue; // Continue processing other files
        }

        // Determine file type
        if (S_ISDIR(file_stat.st_mode)) {
            printf("d");
        } else if (S_ISREG(file_stat.st_mode)) {
            printf("-");
        } else {
            printf("?"); // For special files
        }

        // Print access permissions for owner
        printf("%c", (file_stat.st_mode & S_IRUSR) ? 'r' : '-');
        printf("%c", (file_stat.st_mode & S_IWUSR) ? 'w' : '-');
        printf("%c", (file_stat.st_mode & S_IXUSR) ? 'x' : '-');
        
        // Print access permissions for group
        printf("%c", (file_stat.st_mode & S_IRGRP) ? 'r' : '-');
        printf("%c", (file_stat.st_mode & S_IWGRP) ? 'w' : '-');
        printf("%c", (file_stat.st_mode & S_IXGRP) ? 'x' : '-');
        
        // Print access permissions for others
        printf("%c", (file_stat.st_mode & S_IROTH) ? 'r' : '-');
        printf("%c", (file_stat.st_mode & S_IWOTH) ? 'w' : '-');
        printf("%c", (file_stat.st_mode & S_IXOTH) ? 'x' : '-');

        // Number of links
        printf(" %2u", file_stat.st_nlink);

        // Get owner and group information
        struct passwd* owner_info = getpwuid(file_stat.st_uid);
        struct group* group_info = getgrgid(file_stat.st_gid);

        // Print owner and group names
        printf(" %-8s %-8s", 
               (owner_info != NULL) ? owner_info->pw_name : "unknown",
               (group_info != NULL) ? group_info->gr_name : "unknown");

        // File size (only for regular files)
        if (S_ISREG(file_stat.st_mode)) {
            printf(" %8lld", (long long)file_stat.st_size);
        } else {
            printf(" %8s", ""); // Empty field for directories
        }

        // Modification date
        char time_buffer[20];
        struct tm* time_info = localtime(&file_stat.st_mtime);
        strftime(time_buffer, sizeof(time_buffer), "%b %d %H:%M", time_info);
        printf(" %s", time_buffer);

        // File name (only the last part of the path)
        char* basename = strrchr(filename, '/');
        if (basename != NULL) {
            basename++; // Skip the '/' character
        } else {
            basename = filename;
        }
        printf(" %s\n", basename);
    }

    return 0;
}