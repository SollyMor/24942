#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#define BUFFER_SIZE 256

typedef struct Node_s {
    char *string;
    struct Node_s *next;
} Node;

Node *head, *tail;

void init_list() {
    head = malloc(sizeof(Node));

    head->string = NULL;
    head->next = NULL;

    tail = head;
}

void free_list() {
    Node* ptr = head;
    while (ptr != NULL) {
        Node* tmp = ptr;
        ptr = ptr->next;
        if (tmp->string != NULL) {
            free(tmp->string);
        }
        free(tmp);
    }
    head = NULL;
    tail = NULL;
}

void push(char *string) {
    unsigned long len = strlen(string) + 1;
    char *copy_ptr = calloc(len, sizeof(char));
    strcpy(copy_ptr, string);

    tail->string = copy_ptr;
    tail->next = calloc(1, sizeof(Node));

    tail = tail->next;
}

void print_list() {
    Node *ptr = head;
    while (ptr != NULL) {
        if (ptr->string) {
            printf("%s\n", ptr->string);
        }
        ptr = ptr->next;
    }
}

void remove_esc_sequences(char *s) {
    char *src = s, *dst = s;

    while (*src) {
        if (*src == '\x1B') { // ESC
            src++; // skip ESC
            if (*src == '[') {
                src++;
                // skip to the letters A–Z or ~
                while (*src && !isalpha((unsigned char)*src) && *src != '~') {
                    src++;
                }
                if (*src)
                    src++; // skip finishing character
            }
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}


int main() {
    char input_buffer[BUFFER_SIZE] = {0};

    init_list();

    while (fgets(input_buffer, BUFFER_SIZE, stdin) != NULL) {
        if (input_buffer[0] == '.') {
            print_list();
            free_list();
            return 0;
        }

        char *end = strchr(input_buffer, '\n');
        if (end == NULL) {
            size_t new_buffer_cnt = 0;
            size_t new_buffer_cap = BUFFER_SIZE;
            char *new_buffer = malloc(BUFFER_SIZE);

            memcpy(new_buffer, input_buffer, BUFFER_SIZE);
            new_buffer_cnt += BUFFER_SIZE - 1;

            while (fgets(input_buffer, BUFFER_SIZE, stdin) != NULL) {
                new_buffer_cap += BUFFER_SIZE;
                new_buffer = realloc(new_buffer, new_buffer_cap);

                memcpy(new_buffer + new_buffer_cnt, input_buffer, BUFFER_SIZE);
                new_buffer_cnt += BUFFER_SIZE - 1;

                end = strchr(new_buffer, '\n');
                if (end) {
                    *end = '\0';
                    remove_esc_sequences(new_buffer);
                    push(new_buffer);
                    free(new_buffer);
                    break;
                }
            }
        } else {
            *end = '\0';
            remove_esc_sequences(input_buffer);
            push(input_buffer);
        }
    }

    return 0;
}