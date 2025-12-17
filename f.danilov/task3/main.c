#include <stdio.h> //НЕ СДАЛ
#include <stdlib.h>
#include <unistd.h> // getuid, setuid и тд (системные вызовы)
#define llu unsigned long long


int user_ids() {
    printf("Реальный UID: %d\n", getuid());  // user id
    printf("Эффективный UID: %d\n\n", geteuid());
    if (getuid() != geteuid()){
        return 1;
    }
    return 0;
}

int new_ids(){
    printf("=== ПОПЫТКА УСТАНОВИТЬ EUID = RUID ===\n");
    if (setuid(getuid()) == -1) {
        return 1;
    }
    return 0;
}

void open_file(const char *file_name){
    printf("Пытаемся открыть файл '%s'...\n", file_name);
    FILE *file = fopen(file_name, "r");
    if (file == NULL) perror("Файл не удалось открыть");
    else {
        printf("Файл успешно открыт!\n");
        fclose(file); // закрываем файл при удачном открытии
    }
    printf("\n");
}  

int main(int argc, char *argv[]) {
    if (argc < 2){
        fprintf(stderr, "Использование: %s <имя_файла>\n", argv[0]);
        fprintf(stderr, "Введите название файла для открытия\n");
        exit(EXIT_FAILURE);
    }

    const char * file_name = argv[1];

    printf("=== ДО УСТАНОВКИ ID ===\n");
    int flag = user_ids();
    open_file(file_name);
    if (flag){
        if (new_ids() == 1){
            perror("Ошибка при setuid\n\n");
        } else {
            printf("ID успешно установлены!\n\n");
        }
    }
    
    
    
    printf("=== ПОСЛЕ УСТАНОВКИ ID ===\n");
    user_ids();
    open_file(file_name);

    return 0;
}