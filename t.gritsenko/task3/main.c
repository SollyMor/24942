#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>



int main(int argc, char* argv[]) {

    FILE* f;
    uid_t uid = getuid();
    uid_t euid = geteuid();
    printf("Real UID: %u\n", uid);
    printf("Effective UID: %u\n", euid);

    if ((f = fopen(argv[1], "r")) != NULL) {
        printf("File was opened succesfully.\n\n");
        fclose(f);
    } else {
        perror("Couldn't open file");
    }

    setuid(uid);

    uid = getuid();
    euid = geteuid();
    printf("Real UID: %u\n", uid);
    printf("Effective UID: %u\n", euid);

    if ((f = fopen(argv[1], "r")) != NULL) {
        printf("File was opened succesfully.\n");
        fclose(f);
    } else {
        perror("Couldn't open file");
    }

    return 0;
}