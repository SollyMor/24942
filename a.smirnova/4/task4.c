#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Structure for linked list node
struct Node {
    char *data;         
    struct Node *next;  
};

int main() {
    struct Node *head = NULL;  // Head of the linked list
    struct Node *current = NULL;  // Current node pointer
    char *input = NULL;       // Dynamic buffer for input
    size_t input_size = 0;    // Size of allocated buffer
    ssize_t bytes_read;       // Number of bytes read by getline
    
    printf("Enter strings:\n");
    
    while (1) {
        printf("> ");
        
        // Read line dynamically with getline
        bytes_read = getline(&input, &input_size, stdin);
        
        if (bytes_read == -1) {
            printf("Error reading input!\n");
            break;
        }
        
        if (input[0] == '.') {
            break;
        }

        // Remove newline character if present
        if (bytes_read > 0 && input[bytes_read-1] == '\n') {
            input[bytes_read-1] = '\0';
            bytes_read--;
        }
        
        struct Node *newNode = (struct Node*)malloc(sizeof(struct Node));
        if (newNode == NULL) {
            printf("Memory allocation failed!\n");
            return 1;
        }
        
        // Allocate memory for string data
        newNode->data = (char*)malloc(bytes_read + 1);
        if (newNode->data == NULL) {
            printf("Memory allocation failed!\n");
            free(newNode);
            return 1;
        }
        
        // Copy string to allocated memory
        strcpy(newNode->data, input);
        newNode->next = NULL;
        
        
        if (head == NULL) {
            head = newNode;
            current = newNode;
        } else {
            current->next = newNode;
            current = newNode;
        }
    }
    
    
    printf("\nAll entered strings:\n");
    current = head;
    while (current != NULL) {
        printf("%s\n", current->data);
        current = current->next;
    }
    
    
    current = head;
    while (current != NULL) {
        struct Node *temp = current;
        current = current->next;
        free(temp->data);
        free(temp);
    }
    
    // Free input buffer
    if (input != NULL) {
        free(input);
    }
    
    return 0;
}