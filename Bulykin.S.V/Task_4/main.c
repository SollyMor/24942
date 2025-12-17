#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Node {
  char *data;
  struct Node *next;
};

struct Node *create_node(char *data) {
  struct Node *node = (struct Node *)malloc(sizeof(struct Node));
  if (node == NULL) {
    perror("Failed to allocate memory");
    return NULL;
  }
  node->data = data;
  node->next = NULL;
  return node;
}

void print_list(struct Node *head) {
  struct Node *current = head;
  while (current != NULL) {
    printf("%s\n", current->data);
    current = current->next;
  }
}

void free_list(struct Node *head) {
  struct Node *current = head;
  while (current != NULL) {
    struct Node *next = current->next;
    free(current->data);
    free(current);
    current = next;
  }
}

int main() {
  int bytes_read;
  char buffer[1024];
  struct Node *head = NULL;
  while (1) {
    printf("> ");
    buffer[0] = '\0';
    bytes_read = 0;
    if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
      if (feof(stdin)) {
        printf("\nEnd of input (EOF)\n");
        break;
      } else {
        perror("fgets failed");
        break;
      }
    }
    if (buffer[0] == '.') {
      break;
    }
    bytes_read = strlen(buffer);
    if (bytes_read > 0 && buffer[bytes_read - 1] == '\n') {
      buffer[bytes_read - 1] = '\0';
      bytes_read--;
    }
    char *input_copy = malloc(bytes_read + 1);
    if (input_copy == NULL) {
      perror("Failed to allocate memory!");
      break;
    }
    strncpy(input_copy, buffer, bytes_read);
    input_copy[bytes_read] = '\0';
    struct Node *node = create_node(input_copy);
    if (node == NULL) {
      perror("Failed to create node!");
      break;
    }
    node->next = head;
    head = node;
  }
  print_list(head);
  free_list(head);
  return 0;
}