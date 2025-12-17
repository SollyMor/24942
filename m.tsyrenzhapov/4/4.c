#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
    char *data; // указатель на строку
    struct Node *next; // указатель на след узел
};

// фильтрует стрелочки и т.д.
void filter_escapes(char *str){
    char *s = str, *d = str;
    while (*s){
        if (*s == 27 || (*s == '^' && s[1] == '[')){ // нашли esc или ^[
            if (*s == 27) s++; // пропускаем ESC
            if (*s == '^') s += 2; // пропускаем ^[
            else if (*s == '[') s++; // пропускаем [
            while (*s && (*s < 'A' || *s > 'Z') && (*s < 'a' || *s > 'z')) s++; // пропускаем цифры
            if (*s) s++; // пропускаем завершающую букву
        }
        else{
            *d++ = *s++; // копируем нормальный символ
        }
    }
    *d = '\0';
}

int main(){
    struct Node *head = NULL, *tail = NULL;
    char buf[1024];
    
    printf("Вводите строки (для завершения введите '.' в начале строки):\n");
    
    while (fgets(buf, sizeof(buf), stdin) && buf[0] != '.'){
        filter_escapes(buf);
        if (strlen(buf) > 1 || (strlen(buf) == 1 && buf[0] != '\n')){ // пропускаем пустые строки
            struct Node *node = malloc(sizeof(struct Node)); //выделяем память под один узел списка
            node->data = malloc(strlen(buf) + 1); //выделяем память в node->data
            strcpy(node->data, buf); //копируем в ode->data строку из buf
            node->next = NULL;
            
            if (!head) head = tail = node;
            else tail = tail->next = node;
        }
    }
    
    printf("\nВведенные строки:\n");
    for (struct Node *curr = head; curr; curr = curr->next)
        printf("%s", curr->data);
    
    while (head){
        struct Node *next = head->next;
        free(head->data);
        free(head);
        head = next;
    }
    
    return 0;
}