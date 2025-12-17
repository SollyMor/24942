#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <termios.h>
#include <unistd.h>

using namespace std;

struct Node {
    char* data;
    Node* next;
};

void setTerminalMode(bool enableRawMode) {
    static struct termios original, modified;
    static bool initialized = false;

    if (!initialized) {
        tcgetattr(STDIN_FILENO, &original);
        modified = original;
        initialized = true;
    }

    if (enableRawMode) {
        modified.c_lflag &= ~(ICANON | ECHO);
        modified.c_cc[VMIN] = 1;  
        modified.c_cc[VTIME] = 0; 
        tcsetattr(STDIN_FILENO, TCSANOW, &modified);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }
}


bool readLineSafe(char* buffer, int bufferSize) {
    int pos = 0;
    char c;
    
    setTerminalMode(true); 
    
    while (pos < bufferSize - 1) {
        if (read(STDIN_FILENO, &c, 1) != 1) {
            break;
        }
        
        if (c == 127 || c == 8) { // Backspace или Delete
            if (pos > 0) {
                pos--;
                cout << "\b \b" << flush;
            }
        }
        else if (c == 21) { // Ctrl+U - очистка строки
            while (pos > 0) {
                pos--;
                cout << "\b \b" << flush;
            }
        }
        else if (c == 27) {
            char seq[2];
            if (read(STDIN_FILENO, &seq[0], 1) == 1) {
                if (read(STDIN_FILENO, &seq[1], 1) == 1) {
                }
            }
        }
        else if (c == '\n' || c == '\r') { // Enter
            break;
        }
        else if (c == 4) { // Ctrl+D - конец файла
            setTerminalMode(false);
            return false;
        }
        else if (c >= 32 && c <= 126) { 
            buffer[pos++] = c;
            cout << c << flush; 
        }
    }
    
    setTerminalMode(false); 
    
    buffer[pos] = '\0';
    cout << endl; 
    
    return true;
}

void appendNode(Node** head, Node** tail, const char* str) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode == nullptr) {
        cerr << "Ошибка выделения памяти для узла" << endl;
        return;
    }
    
    size_t len = strlen(str);
    newNode->data = (char*)malloc(len + 1);
    if (newNode->data == nullptr) {
        cerr << "Ошибка выделения памяти для строки" << endl;
        free(newNode);
        return;
    }
    
    strcpy(newNode->data, str);
    newNode->next = nullptr;

    if (*head == nullptr) {
        *head = *tail = newNode;
    } else {
        (*tail)->next = newNode;
        *tail = newNode;
    }
}

void printList(Node* head) {
    Node* current = head;
    while (current != nullptr) {
        cout << current->data << endl;
        current = current->next;
    }
}

void freeList(Node* head) {
    Node* current = head;
    while (current != nullptr) {
        Node* next = current->next;
        free(current->data);
        free(current);
        current = next;
    }
}

int main() {
    Node* head = nullptr;
    Node* tail = nullptr;
    const int BUFFER_SIZE = 1024;
    char buffer[BUFFER_SIZE];
    
    cout << "Введите строки (только печатные символы)." << endl;
    cout << "Доступные команды:" << endl;
    cout << "  - Backspace: удалить последний символ" << endl;
    cout << "  - Ctrl+U: очистить всю строку" << endl;
    cout << "  - Enter: завершить ввод строки" << endl;
    cout << "  - Ctrl+D или точка в начале строки: завершить программу" << endl;
    cout << "----------------------------------------" << endl;
    
    while (true) {
        cout << "> " << flush;
        
        if (!readLineSafe(buffer, BUFFER_SIZE)) {
            cout << "Завершение программы (Ctrl+D)" << endl;
            break;
        }
        
        size_t len = strlen(buffer);
        if (len > 0 && buffer[0] == '.') {
            cout << "Завершение программы (точка)" << endl;
            break;
        }
        
        if (len > 0) {
            appendNode(&head, &tail, buffer);
        }
    }

    cout << "\nСодержание списка:" << endl;
    cout << "----------------------------------------" << endl;
    printList(head);
    
    freeList(head);
    
    return 0;
}

// g++ -o string_list string_list.cpp
// ./string_list
