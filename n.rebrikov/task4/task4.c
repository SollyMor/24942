#include <stdio.h>
#include <string.h>
#include <stdlib.h>


//delete escape sequences from string
void remove_escape_sequences(char *str)
{
int i = 0;
int j = 0;
while(str[i] != '\0')
{
//if found ESC symbol
if( (unsigned char)str[i] == '\x1B') // or 27
	{
//Skip ESC and next symnols until character
i++; // skip ESC
while( str[i] != '\0' &&
	str[i] != 'A' &&
	str[i] != 'B' && str[i] != 'B' &&
	str[i] != 'C' &&
	str[i] != 'D' &&
	str[i] != '~')
{
i++;
}
if (str[i] != '\0') i++;

	}
else
   {
//copy simple symb
	str[j] = str[i];
	j++;
	i++;

   }

}

str[j] = '\0'; //end string

}


struct Node
{
    char *data; //строка
    struct Node* pnext; //указатель на следующий узел
};

//Добавление строки в список
struct Node* add_node(struct Node* head, const char* str)
{
    struct Node* new_node = (struct Node*)malloc(sizeof(struct Node));
    if(new_node == NULL)
    {
        fprintf(stderr,"Error allocating memory for node");
        exit(1);
    }

    new_node->data = (char*)malloc(strlen(str) + 1);
    if(new_node->data == NULL)
    {
        fprintf(stderr,"Error allocating memory for string\n");
        free(new_node);
        exit(1);
    }

    strcpy(new_node->data,str);
    new_node->pnext = NULL; //добавляем в конец списка

    if(head == NULL)
    {
        return new_node; //первый элемент
    }
    
    //Ищем последний элемент
    struct Node* current = head;
    while(current->pnext != NULL)
    {
        current = current->pnext;
    }
    current->pnext = new_node;


    return head;
}

//Напечатать строки
void print_strings(struct Node* head)
{
    struct Node* current = head;
    int count = 1;
    
    printf("\n--- Output all lines ---\n");
    while(current != NULL)
    {
        printf("%d: %s",count,current->data);
        current = current->pnext;
        count++;
    }

}

void free_list(struct Node* head)
{
    struct Node* current = head;
    while(current != NULL)
    {
        struct Node* next = current->pnext;
        free(current->data);
        free(current);
        current = next;
    }
}

int main()
{
    char buffer[256];
    struct Node* head = NULL;

    printf("Enter lines (to complete, enter '.' at the beginning of the line):\n");

    while(1)
    {
        if(fgets(buffer, sizeof(buffer), stdin) == NULL)
        {
            break;
        }

        if(buffer[0] == '.')
        {
            break;
        }
        
        head = add_node(head,buffer);
    }

    print_strings(head);
    free_list(head);

    return 0;
}
