#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BUFFER_SIZE 4096
#define LETTERS_ONLY 0

typedef struct Node_s
{
    char *string;
    struct Node_s *next;
} Node;

static Node *head = NULL, *tail = NULL;

static void push(const char *s)
{
    size_t len = strlen(s) + 1;
    char *copy = (char *)malloc(len);
    if (!copy)
    {
        perror("malloc");
        exit(1);
    }
    memcpy(copy, s, len);

    Node *node = (Node *)malloc(sizeof(Node));
    if (!node)
    {
        perror("malloc");
        exit(1);
    }
    node->string = copy;
    node->next = NULL;

    if (!tail)
        head = tail = node;
    else
    {
        tail->next = node;
        tail = node;
    }
}

static void printList(void)
{
    for (Node *p = head; p; p = p->next)
        puts(p->string);
}

static void freeList(void)
{
    Node *p = head;
    while (p)
    {
        Node *n = p->next;
        free(p->string);
        free(p);
        p = n;
    }
    head = tail = NULL;
}

static void sanitize_line(char *s)
{
    char *r = s;
    char *w = s;

    while (*r)
    {
        unsigned char c = (unsigned char)*r;

        if (c == 0x1B)
        {
            r++;
            if (*r == '[' || *r == 'O')
            {
                r++;
                while (*r)
                {
                    unsigned char d = (unsigned char)*r;
                    if (d >= 0x40 && d <= 0x7E)
                    {
                        r++;
                        break;
                    }
                    r++;
                }
                continue;
            }
            continue;
        }

        if (c == '\r')
        {
            r++;
            continue;
        }

        if (c < 0x20 || c == 0x7F)
        {
            r++;
            continue;
        }

        if (LETTERS_ONLY)
        {
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == ' ')
            {
                *w++ = (char)c;
            }
        }
        else
        {
            if (c >= 0x20 && c <= 0x7E)
            {
                *w++ = (char)c;
            }
        }

        r++;
    }
    *w = '\0';
}

int main(void)
{
    char buf[BUFFER_SIZE];

    while (fgets(buf, sizeof buf, stdin))
    {
        char *nl = strchr(buf, '\n');
        if (nl)
            *nl = '\0';

        if (buf[0] == '.' && buf[1] == '\0')
            break;

        sanitize_line(buf);

        if (buf[0] == '\0')
            continue;

        push(buf);
    }

    printList();
    freeList();
    return 0;
}

