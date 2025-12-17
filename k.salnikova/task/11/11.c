#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>

extern char **environ;

char* find_executable_in_path(const char *file, char *const envp[]) {
    if (strchr(file, '/') != NULL) {
        if (access(file, X_OK) == 0) {
            return strdup(file);
        }
        return NULL;
    }

    char *path_env = NULL;
    
    // Сначала ищем PATH в переданном окружении
    if (envp != NULL) {
        for (int i = 0; envp[i] != NULL; i++) {
            if (strncmp(envp[i], "PATH=", 5) == 0) {
                path_env = envp[i] + 5;
                break;
            }
        }
    }
    
    // Если не нашли в envp, используем текущее окружение (environ)
    if (path_env == NULL) {
        for (int i = 0; environ[i] != NULL; i++) {
            if (strncmp(environ[i], "PATH=", 5) == 0) {
                path_env = environ[i] + 5;
                break;
            }
        }
    }
    
    // Если всё ещё не нашли, используем стандартный PATH
    if (path_env == NULL) {
        path_env = "/bin:/usr/bin:/usr/local/bin:/usr/sbin";
    }

    char *path_copy = strdup(path_env);
    if (path_copy == NULL) {
        return NULL;
    }

    char *full_path = NULL;
    char *saveptr = NULL;
    char *dir = strtok_r(path_copy, ":", &saveptr);

    while (dir != NULL) {
        size_t len = strlen(dir) + strlen(file) + 2;
        full_path = malloc(len);
        if (full_path == NULL) {
            free(path_copy);
            return NULL;
        }

        snprintf(full_path, len, "%s/%s", dir, file);

        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }

        free(full_path);
        full_path = NULL;
        dir = strtok_r(NULL, ":", &saveptr);
    }

    free(path_copy);
    return NULL;
}

int execvpe(const char *file, char *const argv[], char *const envp[]) {
    if (file == NULL || argv == NULL) {
        errno = EINVAL;
        return -1;
    }

    char *full_path = find_executable_in_path(file, envp);
    if (full_path == NULL) {
        errno = ENOENT;
        return -1;
    }

    // Всегда используем execve с правильным окружением
    if (envp == NULL) {
        execve(full_path, argv, environ);
    } else {
        execve(full_path, argv, envp);
    }

    int saved_errno = errno;
    free(full_path);
    errno = saved_errno;
    return -1;
}

// Тестовая функция main
int main() {
    printf("=== Testing execvpe() function on Solaris ===\n\n");

    // Тест 1: с текущим окружением
    printf("Test 1: Using current environment\n");
    printf("Executing: /usr/bin/ls -l\n");
    
    char *args1[] = {"ls", "-l", NULL};
    pid_t pid1 = fork();
    
    if (pid1 == 0) {
        if (execvpe("ls", args1, NULL) == -1) {
            perror("execvpe failed in test 1");
            exit(1);
        }
    } else if (pid1 > 0) {
        wait(NULL);
        printf("Test 1 completed successfully\n\n");
    } else {
        perror("fork failed");
    }

    // Тест 2: с кастомным окружением
    printf("Test 2: Using custom environment\n");
    
    char *args2[] = {"echo", "Hello Solaris!", NULL};
    char *env2[] = {
        "PATH=/bin:/usr/bin",
        "TERM=vt100",
        NULL
    };
    
    pid_t pid2 = fork();
    
    if (pid2 == 0) {
        if (execvpe("echo", args2, env2) == -1) {
            perror("execvpe failed in test 2");
            exit(1);
        }
    } else if (pid2 > 0) {
        wait(NULL);
        printf("Test 2 completed successfully\n\n");
    } else {
        perror("fork failed");
    }

    printf("=== All tests completed ===\n");
    return 0;
}