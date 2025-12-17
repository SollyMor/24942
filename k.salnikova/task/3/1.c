#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

int main() {
    const char *filename = "secret_file.txt";
    
    printf("=== ДО setuid ===\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    
    if (access(filename, F_OK) == -1) {
        fprintf(stderr, "Файл %s не существует\n", filename);
        return 1;
    }
    
    if (access(filename, R_OK) == -1) {
        perror("Нет прав на чтение файла");
    }
    
    FILE *file = fopen(filename, "rb");
    if (file != NULL) {
        printf(" Файл открыт успешно\n");
        if (fclose(file) == EOF) {
            perror("Ошибка закрытия файла");
        }
    } else {
        perror(" Ошибка открытия файла");
    }
    
    printf("\n=== Устанавливаем setuid(getuid()) ===\n");
    if (setuid(getuid()) == -1) {
        perror("Критическая ошибка setuid");
        return 1; 
    }
    
    printf("\n=== ПОСЛЕ setuid ===\n");
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    
    file = fopen(filename, "rb");
    if (file != NULL) {
        printf(" Файл открыт успешно\n");
        if (fclose(file) == EOF) {
            perror("Ошибка закрытия файла");
        }
    } else {
        perror(" Ошибка открытия файла");
    }
    
    return 0;
}

/*
 gcc -o 1 1.c
./1
*/  