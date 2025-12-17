#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // read, lseek, close, alarm, write
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>     // sigaction
#include <string.h>     // strlen, strchr
#include <ctype.h>      // isspace
#include <errno.h>

#define MAX_LINES 10000
#define MAX_LINE_LENGTH 256
#define TIMEOUT 5 // таймер только на первый ввод

typedef struct{
    long offset;
    int  length;
} LineInfo;

volatile sig_atomic_t timeout_occurred = 0;
int global_fd = -1;

/* ---------- безопасный обработчик сигнала ---------- */
static void alarm_handler(int sig){
    (void)sig;
    timeout_occurred = 1; // только флаг!
}

/* ---------- печать всего файла (используется при таймауте) ---------- */
static void print_entire_file(void){
    if (global_fd == -1) return;
    if (lseek(global_fd, 0L, SEEK_SET) == -1){ perror("lseek"); return; }

    char buffer[1024];
    ssize_t n;

    puts("\n\nВремя на ввод истекло! Выводим содержимое файла:");
    puts("_____ПОЛНОЕ СОДЕРЖИМОЕ ФАЙЛА_____");
    while ((n = read(global_fd, buffer, sizeof buffer)) > 0){
        if (write(STDOUT_FILENO, buffer, (size_t)n) < 0){ perror("write"); break; }
    }
    if (n < 0) perror("read");
    puts("\n_____КОНЕЦ ФАЙЛА_____");
}

/* ---------- утилиты ввода ---------- */
static void trim(char **pb, char **pe){
    char *b=*pb, *e=*pe;
    while (b<e && isspace((unsigned char)*b)) b++;
    while (e>b && isspace((unsigned char)e[-1])) e--;
    *pb=b; *pe=e;
}

/* чтение числа С ТАЙМЕРОМ (только первый ввод)
   1 — успех, 0 — EOF, -1 — мусор, -2 — таймаут */
static int read_int_with_timeout(long min,long max,long *out){
    char buf[64];
    timeout_occurred = 0;
    alarm(TIMEOUT);
    char *res = fgets(buf, sizeof buf, stdin);
    alarm(0); // выключаем таймер сразу после возврата

    if (!res){
        if (timeout_occurred) return -2;   // сработал alarm
        return 0;                          // EOF/ошибка
    }
    if (!strchr(buf,'\n')){ int c; while ((c=getchar())!='\n' && c!=EOF){} }

    char *p=buf, *e=buf+strlen(buf);
    trim(&p,&e); *e='\0';
    if (*p=='\0'){ fprintf(stderr,"Пустой ввод. Повторите.\n"); return -1; }

    errno=0; char *q=NULL; long v=strtol(p,&q,10);
    if (errno==ERANGE){ fprintf(stderr,"Число вне диапазона long.\n"); return -1; }
    if (q==p || *q!='\0'){ fprintf(stderr,"Ожидалось целое число.\n"); return -1; }
    if (v<min || v>max){ fprintf(stderr,"Число должно быть от %ld до %ld.\n",min,max); return -1; }

    *out=v; return 1;
}

/* чтение числа БЕЗ таймера (все последующие вводы)
   1 — успех, 0 — EOF, -1 — мусор */
static int read_int_safe(long min,long max,long *out){
    char buf[64];
    if (!fgets(buf, sizeof buf, stdin)) return 0; // EOF
    if (!strchr(buf,'\n')){ int c; while ((c=getchar())!='\n' && c!=EOF){} }

    char *p=buf, *e=buf+strlen(buf);
    trim(&p,&e); *e='\0';
    if (*p=='\0'){ fprintf(stderr,"Пустой ввод. Повторите.\n"); return -1; }

    errno=0; char *q=NULL; long v=strtol(p,&q,10);
    if (errno==ERANGE){ fprintf(stderr,"Число вне диапазона long.\n"); return -1; }
    if (q==p || *q!='\0'){ fprintf(stderr,"Ожидалось целое число.\n"); return -1; }
    if (v<min || v>max){ fprintf(stderr,"Число должно быть от %ld до %ld.\n",min,max); return -1; }

    *out=v; return 1;
}

int main(int argc, char *argv[]){
    int fd;
    char ch;
    LineInfo lines[MAX_LINES];
    int line_count = 0;
    int line_length = 0;

    if (argc != 2){
        fprintf(stderr,"Использование: %s <filename>\n", argv[0]);
        return 1;
    }
    fd = open(argv[1], O_RDONLY);
    if (fd == -1){ perror("Ошибка открытия файла"); return 1; }
    global_fd = fd;

    printf("Файл '%s' успешно открыт\n", argv[1]);

    /* индекс строк */
    printf("\n===ПОСТРОЕНИЕ ТАБЛИЦЫ СТРОК===\n");
    lines[0].offset = 0;

    ssize_t r;
    while ((r = read(fd, &ch, 1)) > 0){
        line_length++;
        if (ch == '\n'){
            if (line_count < MAX_LINES){
                lines[line_count].length = line_length - 1;
                printf("Строка %d: смещение = %ld, длина = %d\n",
                       line_count + 1, lines[line_count].offset, lines[line_count].length);
                line_count++;
                if (line_count < MAX_LINES){
                    off_t cur = lseek(fd, 0L, SEEK_CUR);
                    if (cur != (off_t)-1) lines[line_count].offset = (long)cur;
                } else {
                    fprintf(stderr,"Достигнут лимит %d строк. Остальные игнорируются.\n", MAX_LINES);
                }
            }
            line_length = 0;
        }
    }
    if (r < 0){ perror("Ошибка чтения файла"); close(fd); return 1; }

    if (line_length > 0 && line_count < MAX_LINES){
        lines[line_count].length = line_length;
        printf("Строка %d: смещение = %ld, длина = %d\n",
               line_count + 1, lines[line_count].offset, lines[line_count].length);
        line_count++;
    }

    printf("\nВсего строк в файле: %d\n", line_count);
    if (line_count == 0){ puts("Файл пуст — выход."); close(fd); return 0; }

    /* сигналы: однократный таймер на первый ввод */
    struct sigaction sa;
    sa.sa_handler = alarm_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // без SA_RESTART, чтобы fgets мог прерваться
    if (sigaction(SIGALRM, &sa, NULL) == -1){ perror("sigaction"); close(fd); return 1; }

    /* ---- ПЕРВЫЙ ВВОД С ТАЙМЕРОМ ---- */
    printf("\n===Интерактивный режим===\n");
    printf("У вас есть %d секунд чтобы ввести номер строки.\n", TIMEOUT);
    printf("Введите номер строки (1-%d) или 0 для выхода:\n> ", line_count);
    fflush(stdout);

    long ln;
    for(;;){
        int rc = read_int_with_timeout(0, line_count, &ln);
        if (rc == -2 || timeout_occurred){  // таймаут
            print_entire_file();
            close(fd);
            return 0;
        } else if (rc == 0){                // EOF
            puts("\nВыход (EOF)");
            close(fd);
            return 0;
        } else if (rc < 0){                 // мусор
            printf("> "); fflush(stdout);
            continue;
        }
        break; // получили корректное число
    }
    if (ln == 0){ puts("Выход из программы"); close(fd); return 0; }

    /* ---- ДАЛЕЕ БЕЗ ТАЙМЕРА: бесконечный выбор ---- */
    for (;;){
        int index = (int)ln - 1;
        long offset = lines[index].offset;
        int  length = lines[index].length;

        printf("\nСтрока %ld: смещение = %ld, длина = %d\n", ln, offset, length);
        printf("Содержимое: ");

        if (lseek(fd, offset, SEEK_SET) == -1){
            perror("ошибка позиционирования");
        } else {
            char buffer[MAX_LINE_LENGTH + 1];
            int to_read = (length < MAX_LINE_LENGTH) ? length : MAX_LINE_LENGTH;
            ssize_t bytes_read = read(fd, buffer, to_read);
            if (bytes_read < 0){
                perror("Ошибка чтения строки");
            } else {
                buffer[bytes_read] = '\0';
                printf("'%s'%s\n", buffer, (length > to_read) ? " ...[обрезано]" : "");
            }
        }

        printf("\nВведите номер строки (1-%d) или 0 для выхода:\n> ", line_count);
        fflush(stdout);

        int rc2 = read_int_safe(0, line_count, &ln);
        if (rc2 == 0){ puts("\nВыход (EOF)"); break; }
        if (rc2 < 0){ continue; }     // мусор — спросим ещё раз
        if (ln == 0){ puts("Выход из программы"); break; }
    }

    close(fd);
    return 0;
}
