#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>

void printIds(const char* message) 
{
    printf("%s\n", message);
    printf("Real UID: %d\n", getuid());
    printf("Effective UID: %d\n", geteuid());
    printf("Real GID: %d\n", getgid());
    printf("Effective GID: %d\n", getegid());
    printf("==================================\n");
}

void tryOpenFile(const char* filename) 
{
    FILE* file = fopen(filename, "r");
    if (file == NULL) 
    {
        perror("fopen failed");
    } 
    else 
    {
        printf("File opened successfully in mode '%s'\n", "r");
        fclose(file);
    }
    printf("==================================\n");
}

int main(int argc, char**argv) 
{
    if (argc < 2)
    {
        perror("Filename is not specified\n");
        return 1;
    }
    
    //До setuid
    printIds("before setuid");
    tryOpenFile(argv[1]);

    if (setuid(geteuid()) == -1) 
    {
        perror("setuid failed");
        return 1;
    }

    printIds("After setuid");
    tryOpenFile(argv[1]);
    
    return 0;
}
