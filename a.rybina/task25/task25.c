// Напишите программу, которая создает подпроцесс, взаимодействующий с родителем через программный канал.
// Один из процессов выдает в канал текст, состоящий из символов верхнего и нижнего регистров.
// Второй процесс переводит все символы в верхний регистр и выводит полученный текст на терминал. Подсказка: см. toupper(3).

#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>

static ssize_t robust_read(int fd, void *buf, size_t count) {
    for (;;) {
        ssize_t n = read(fd, buf, count);
        if (n >= 0) return n;
        if (errno == EINTR) continue;
        return -1;
    }
}

static ssize_t robust_write(int fd, const void *buf, size_t count) {
    const char *p = (const char *)buf;
    size_t left = count;
    while (left > 0) {
        ssize_t n = write(fd, p, left);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p += (size_t)n;
        left -= (size_t)n;
    }
    return (ssize_t)count;
}

int main(void) {
    printf("Pipe program: Parent sends text, child converts to uppercase.\n");
    printf("Type text (Ctrl+D to finish):\n");
    fflush(stdout);
    
    int pipe_fds[2];
    if (pipe(pipe_fds) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid == 0) {
        // Child: read from pipe, convert to upper-case, write to stdout
        if (close(pipe_fds[1]) == -1) {
            perror("close");
            _exit(1);
        }

        char buffer[8192];
        for (;;) {
            ssize_t n = robust_read(pipe_fds[0], buffer, sizeof(buffer));
            if (n == 0) break;
            if (n < 0) {
                perror("read");
                _exit(1);
            }
            size_t write_pos = 0;
            for (ssize_t i = 0; i < n; ++i) {
                unsigned char ch = (unsigned char)buffer[i];
                // Filter out control characters that might trigger commands
                if (isalnum(ch) || isprint(ch)) {
                    buffer[write_pos] = (char)(isalpha(ch) ? toupper(ch) : ch);
                    write_pos++;
                }
            }
            n = (ssize_t)write_pos;
            if (n > 0) {
                if (robust_write(STDOUT_FILENO, buffer, (size_t)n) < 0) {
                    perror("write");
                    _exit(1);
                }
                // Add newline after output for next input
                if (robust_write(STDOUT_FILENO, "\n", 1) < 0) {
                    perror("write");
                    _exit(1);
                }
            }
        }

        if (close(pipe_fds[0]) == -1) {
            perror("close");
            _exit(1);
        }
        _exit(0);
    }

    // Parent: read stdin and forward to pipe
    if (close(pipe_fds[0]) == -1) {
        perror("close");
        return 1;
    }

    char buffer[8192];
    for (;;) {
        ssize_t n = robust_read(STDIN_FILENO, buffer, sizeof(buffer));
        if (n == 0) break;
        if (n < 0) {
            perror("read");
            break;
        }
        if (robust_write(pipe_fds[1], buffer, (size_t)n) < 0) {
            perror("write");
            break;
        }
    }

    close(pipe_fds[1]);
    (void)waitpid(pid, NULL, 0);
    return 0;
}