// Создайте файл данных, который может писать и читать только владелец (это можно сделать командой shell chmod 600 file) и напишите программу, которая
// 1. Печатает реальный и эффективный идентификаторы пользователя.
// 2. Открывает файл с помощью fopen(3). Если fopen() завершился успешно, файл должен быть закрыт с помощью fclose(3). Напечатайте сообщение об ошибке, используя perror(3C), если файл не удалось открыть.
// 3. Сделайте, чтобы реальный и эффективный идентификаторы пользователя совпадали. Подсказка: setuid(2)
// 4. Повторите первые два шага.

// Проверьте работу вашей программы.
// 1. Исполните программу и посмотрите вывод
// 2. Сделайте программу доступной для запуска членам вашей группы и пусть ваши одногруппники исполнят программу.
// 3. Командой chmod u+s prog установите бит установки идентификатора пользователя и пусть ваши одногруппники опять исполнят эту программу.

//chown !!!

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>


int main(int argc, char *argv[]) {
    // Check if filename is provided
    if (argc != 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    char *filename = argv[1];
    
    uid_t real_uid = getuid();
    uid_t eff_uid = geteuid();

    printf("Real UID: %d\n", real_uid);
    printf("Effective UID: %d\n", eff_uid);

    // open file
    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
    } else {
        fclose(file);
        printf("File opened successfully\n\n");
    }

    // set uid
    setuid(real_uid);

    // repeat first and second step
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());

    file = fopen(filename, "w");
    if (file == NULL) {
        perror("Failed to open file");
        return 1;
    }
    fclose(file);
    printf("File opened successfully 2nd time\n");
    
    return 0;
}

// ls -l file.txt
// chmod u+s task3.exe
// ls -l task3.exe
