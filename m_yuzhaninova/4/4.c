#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef struct Node
{
    char *text;
    struct Node *next;
} Node;

void free_lines(Node *head)
{
    while (head)
    {
        Node *temp = head;
        head = head->next;
        free(temp->text);
        free(temp);
    }
}

void add_line(Node **head, const char *line)
{
    Node *new_node = malloc(sizeof(Node));
    if (!new_node)
    {
        perror("malloc");
        return;
    }

    new_node->text = malloc(strlen(line) + 1);
    if (!new_node->text)
    {
        perror("malloc");
        free(new_node);
        return;
    }

    strcpy(new_node->text, line);
    new_node->next = NULL;

    if (*head == NULL)
    {
        *head = new_node;
    }
    else
    {
        Node *temp = *head;
        while (temp->next)
        {
            temp = temp->next;
        }
        temp->next = new_node;
    }
}

char *read_input()
{
    long size = 100;
    long len = 0;
    char *buf = malloc(size);
    if (!buf)
    {
        perror("malloc");
        return NULL;
    }

    while (fgets(buf + len, size - len, stdin))
    {
        len += strlen(buf + len);

        for (long i = 0; i < len; i++)
        {
            if (!isprint((unsigned char)buf[i]) && buf[i] != '\n')
            {
                printf("Ошибка: введены недопустимые символы!\n");
                buf[0] = '\0';
                return buf;
            }
        }

        if (len > 0 && buf[len - 1] == '\n')
        {
            buf[len - 1] = '\0';
            return buf;
        }

        size *= 2;
        char *new_buf = realloc(buf, size);
        if (!new_buf)
        {
            perror("realloc");
            free(buf);
            return NULL;
        }
        buf = new_buf;
    }

    if (len == 0)
    {
        free(buf);
        return NULL;
    }

    return buf;
}

int main()
{
    Node *list = NULL;
    char *line = NULL;

    printf("Введите строку (. для выхода):\n");

    while ((line = read_input()) != NULL)
    {
        if (strcmp(line, ".") == 0)
        {
            free(line);
            break;
        }

        if (line[0] == '\0')
        {
            free(line);
            continue;
        }

        add_line(&list, line);
        free(line);
    }

    printf("\nВы ввели:\n");
    for (Node *p = list; p; p = p->next)
    {
        printf("%s\n", p->text);
    }

    free_lines(list);
    return 0;
}
