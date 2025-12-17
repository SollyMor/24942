#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  // Check if filename is provided
  if (argc != 2) {
    printf("Usage: %s <filename>\n", argv[0]);
    return 1;
  }

  char *filename = argv[1];

  uid_t real_uid = getuid();
  uid_t eff_uid = geteuid();

  printf("Real UID: %d\n", real_uid);
  printf("Effective UID: %d\n", eff_uid);

  // open file
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    perror("Failed to open file");
  } else {
    fclose(file);
    printf("File opened successfully\n\n");
  }

  // set uid
  setuid(real_uid);

  // repeat first and second step
  printf("Real UID: %d\n", getuid());
  printf("Effective UID: %d\n", geteuid());

  file = fopen(filename, "w");
  if (file == NULL) {
    perror("Failed to open file");
    return 1;
  }
  fclose(file);
  printf("File opened successfully 2nd time\n");

  return 0;
}

// ls -l file.txt
// chmod u+s task3.exe
// ls -l task3.exe
