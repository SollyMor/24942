#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


typedef struct Strings 
{
    char *data;
    struct Strings *next;
}S;


void new_string(S **head, const char *str) 
{
    S *new = (S *)malloc(sizeof(S));

    if (!new) 
    {
        fprintf(stderr, "Ошибка malloc");
        return;
    }

    new->data = (char *)malloc(strlen(str) + 1); 

    if (!new->data) 
    { 
        fprintf(stderr, "Ошибка malloc");
        free(new);
        return;
    }

    strcpy(new->data, str);
    new->next = NULL;

    if (*head == NULL) 
    {
        *head = new;
    } 
    else 
    {
        S *p = *head;
        while (p->next != NULL)
        {
            p = p->next;
        }

        p->next = new;
    }
}


void free_list(S *head) 
{
    while (head) 
    {
        S *tmp = head;
        head = head->next;
        free(tmp->data);
        free(tmp);
    }
}

char *read_line()
{
    long bufsize = 128;
    long len = 0;
    char *buffer = malloc(bufsize);
    if (!buffer) 
    {
        fprintf(stderr, "Ошибка malloc");
        return NULL;
    }

    while (fgets(buffer + len, bufsize - len, stdin)) 
    {
        len += strlen(buffer + len);

        // проверяем, что все символы печатные или пробел
        for (long i = 0; i < len; ++i)
        {
            if (!isprint((unsigned char)buffer[i]) && buffer[i] != '\n')
            {
                printf("Введены недопустимые символы!\n");
                buffer[0] = '\0';
                return buffer;
            }
        }

        if (len > 0 && buffer[len-1] == '\n') 
        {
            buffer[len-1] = '\0'; 
            return buffer;
        }

        bufsize *= 2;
        char *new_buf = realloc(buffer, bufsize);
        if (!new_buf) 
        {
            fprintf(stderr, "Ошибка realloc");
            free(buffer);
            return NULL;
        }

        buffer = new_buf;
    }

    if (len == 0) 
    {
        free(buffer);
        return NULL;
    }

    return buffer;
}

int main() 
{
    S *head = NULL;
    char *str = NULL;

    printf("Введите строку. ('.' для выхода)\n");
    while ((str = read_line()) != NULL)
    {
        if (strcmp(str, ".") == 0) 
        {
            free(str);
            break;
        }

        if (str[0] == '\0')
        {
            continue;
        }
        new_string(&head, str);
        free(str);
    }

    printf("Вы ввели:\n");
    S *p = head;
    while (p) 
    {
        printf("%s\n", p->data);
        p = p->next;
    }

    free_list(head);
    return 0;
}