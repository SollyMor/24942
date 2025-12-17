#include <errno.h>
#include <getopt.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

char *strdup(const char *s) {
  size_t len = strlen(s) + 1;
  char *dup = malloc(len);
  if (dup != NULL) {
    strcpy(dup, s);
  }
  return dup;
}

void print_usage(const char *progname) {
  printf("Usage: %s [options]\n", progname);
  printf("Options:\n");
  printf("  -i    Print real and effective user and group IDs\n");
  printf("  -s    Process becomes group leader\n");
  printf("  -p    Print process, parent process, and process group IDs\n");
  printf("  -u    Print ulimit value\n");
  printf("  -U    Change ulimit value\n");
  printf("  -c    Print core file size limit in bytes\n");
  printf("  -C    Change core file size limit\n");
  printf("  -d    Print current working directory\n");
  printf("  -v    Print environment variables and their values\n");
  printf("  -V    Set environment variable (name=value)\n");
}

void print_user_group_ids() {
  printf("Printing user group ids:\n");
  printf("Real UID: %d\n", getuid());
  printf("Effective UID: %d\n", geteuid());
  printf("Real GID: %d\n", getgid());
  printf("Effective GID: %d\n", getegid());
}

void become_group_leader() {
  printf("Changing process to group leader...\n");
  if (setpgid(0, 0) == -1) {
    perror("setpgid failed");
    return;
  }
  printf("Process became group leader\n");
}

void print_process_ids() {
  printf("Printing process ids:\n");
  printf("Process ID: %d\n", getpid());
  printf("Parent Process ID: %d\n", getppid());
  printf("Process Group ID: %d\n", getpgrp());
}

struct rlimit get_rlimit(int resource) {
  struct rlimit rlim;
  if (getrlimit(resource, &rlim) == -1) {
    perror("getrlimit failed");
    return (struct rlimit){0, 0};
  }
  return rlim;
}

void change_rlimit(int resource, const char *value) {
  printf("Changing rlimit value to %s...\n", value);
  long new_limit = atol(value);
  if (new_limit < 0) {
    fprintf(stderr, "Invalid value: %s\n", value);
    return;
  }

  struct rlimit rlim;
  rlim.rlim_cur = (rlim_t)new_limit;
  rlim.rlim_max = (rlim_t)new_limit;

  if (setrlimit(resource, &rlim) == -1) {
    perror("setrlimit");
    return;
  }
  printf("Rlimit value changed to %ld bytes\n", new_limit);
}

void print_ulimit() {
  printf("Getting file size limit...\n");
  struct rlimit rlim = get_rlimit(RLIMIT_FSIZE);
  printf("File size limit: %ld bytes\n", (long)rlim.rlim_cur);
}

void change_ulimit(const char *value) {
  printf("Changing file size limit to %s...\n", value);
  change_rlimit(RLIMIT_FSIZE, value);
}

void print_core_size() {
  printf("Getting core file size limit...\n");
  struct rlimit rlim = get_rlimit(RLIMIT_CORE);
  printf("Core file size limit: %ld bytes\n", (long)rlim.rlim_cur);
}

void change_core_size(const char *value) {
  printf("Changing core file size limit to %s...\n", value);
  change_rlimit(RLIMIT_CORE, value);
}

void print_current_directory() {
  printf("Getting current directory...\n");
  char cwd[PATH_MAX];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("getcwd failed");
    return;
  }
  printf("Current directory: %s\n", cwd);
}

void print_environment() {
  printf("Getting environment variables...\n");
  extern char **environ;
  char **env = environ;
  printf("Environment variables:\n");
  while (*env != NULL) {
    printf("%s\n", *env);
    env++;
  }
}

void set_environment_variable(const char *name, const char *value) {
  printf("Setting environment variable %s to %s...\n", name, value);
  if (setenv(name, value, 1) == -1) {
    perror("setenv");
    return;
  }
  printf("Environment variable %s set to %s\n", name, value);
}

int parse_name_value(const char *input, char **name, char **value) {
  char *equals = strchr(input, '=');
  if (equals == NULL) {
    perror("Invalid format for -V. Use: name=value\n");
    return -1;
  }
  size_t name_len = equals - input;
  *name = malloc(name_len + 1);
  *value = strdup(equals + 1);

  if (*name == NULL || *value == NULL) {
    perror("malloc/strdup failed");
    free(*name);
    *name = NULL;
    free(*value);
    *value = NULL;
    return -1;
  }

  strncpy(*name, input, name_len);
  (*name)[name_len] = '\0';
  return 0;
}

int main(int argc, char *argv[]) {
  unsigned int op_count = 0;
  int opt = 0;

  while ((opt = getopt(argc, argv, "ispuU:cC:dvV:")) != -1) {
    op_count++;
    switch (opt) {
    case 'i':
      print_user_group_ids();
      break;
    case 's':
      become_group_leader();
      break;
    case 'p':
      print_process_ids();
      break;
    case 'u':
      print_ulimit();
      break;
    case 'U':
      change_ulimit(optarg);
      break;
    case 'c':
      print_core_size();
      break;
    case 'C':
      change_core_size(optarg);
      break;
    case 'd':
      print_current_directory();
      break;
    case 'v':
      print_environment();
      break;
    case 'V': {
      char *name, *value;
      if (parse_name_value(optarg, &name, &value) == -1) {
        break;
      }

      set_environment_variable(name, value);

      free(name);
      free(value);
      break;
    }
    case '?':
      print_usage(argv[0]);
      exit(1);
    default:
      print_usage(argv[0]);
      exit(1);
    }
  }
  if (op_count == 0) {
    print_usage(argv[0]);
    exit(1);
  }

  return 0;
}