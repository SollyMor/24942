#include <stdio.h> //НЕ СДАЛ (исправил)
#include <stdlib.h>
#include <string.h>
#define MAX_LEN 1024 // максимальная длина строки

// структура хранения строк
typedef struct ListNode { 
    char * data;
    struct ListNode *next;
} ListNode;

// Функция для добавлния узла
void add_node(ListNode ** list, char * new_str){
    ListNode * new_node = malloc(sizeof(ListNode)); 
    new_node->data = (char*)malloc(strlen(new_str) + 1);
    if (new_node == NULL || new_node->data == NULL) {
        perror("Ошибка выделения памяти");
        exit(EXIT_FAILURE);
    }

    strcpy(new_node->data, new_str);
    new_node->next = NULL;
    if (*list == NULL) {
        *list = new_node;     
    } else {
        ListNode * curr = *list;
        while (curr->next != NULL){
            curr = curr->next;
        }
        curr->next = new_node;
    }
}

char * read_line(){
    static char buffer[MAX_LEN];
    int pos = 0;
    int ch;

    while (1) {
        ch = getchar();

        // Обработка Enter
        if (ch == '\n' || ch == '\r') {
            buffer[pos] = '\0';
            return buffer;
        }

        if (ch >= 32 && ch <= 126) {
            if (pos < MAX_LEN - 1) {
                buffer[pos++] = (char)ch;
            }
        }
    }
}

// Функция для вывода всех строк из списка
void print_list(ListNode* list) {
    ListNode* current = list;
    int count = 1;
    printf("=== Все строки ===\n");
    while (current != NULL) {
        printf("%d: %s\n", count, current->data);
        current = current->next;
        count++;
    }
}

// Функция для освобождения памяти
int free_memory(ListNode* list){
    ListNode * curr = list;
    while (curr != NULL){
        ListNode* next = curr->next;
        free(curr->data);
        free(curr);
        curr = next;
    }
}

int main(void){
    ListNode* list = NULL;
    char * line;
    printf("Вводите строки (для завершения введите '.' в начале строки):\n");

    while (1){
        line = read_line();

        // проверяем условие завершения (точка в начале строки)
        if (line[0] == '.') {
            break;
        }

        // убираем символ новой строки, если он есть
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        add_node(&list, line); // добавляем новую строку в список
    }
    
    print_list(list); 
    free_memory(list); // очищаем память
    return 0;
}   