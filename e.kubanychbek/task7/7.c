/*
    Индексация строк с отображением файла в память (mmap).
    Таймер SIGALRM — только на первый ввод; далее бесконечный выбор строк.
    Безопасный ввод: fgets + strtol. Доступ к файлу — через mmap (без read/lseek).
*/

#define _XOPEN_SOURCE 700          // просим интерфейсы POSIX.1-2008
#include <stdio.h>                 // printf, puts, fwrite
#include <stdlib.h>                // exit, size_t
#include <unistd.h>                // close, alarm
#include <fcntl.h>                 // open, O_RDONLY
#include <sys/types.h>             // типы для системных вызовов
#include <sys/stat.h>              // fstat, S_ISREG
#include <signal.h>                // sigaction, SIGALRM
#include <sys/mman.h>              // mmap, munmap
#include <string.h>                // strlen, strchr
#include <ctype.h>                 // isspace
#include <errno.h>                 // errno, ERANGE

#define MAX_LINES 10000            // максимум строк, которые индексируем
#define MAX_LINE_LENGTH 256        // максимум символов для печати одной строки
#define TIMEOUT 5                  // секунд на ПЕРВЫЙ ввод пользователя

// Структура для индекса одной строки: смещение и длина (без '\n')
typedef struct {
    long offset;                   // смещение от начала файла (байты)
    int  length;                   // длина строки без символа '\n'
} LineInfo;

// Глобальные для печати целого файла при таймауте
volatile sig_atomic_t timeout_occurred = 0;  // флаг, выставляемый в обработчике SIGALRM
const unsigned char *g_map = NULL;           // указатель на отображение файла (read-only)
size_t g_size = 0;                           // размер файла (в байтах)

// ====== Обработчик SIGALRM: только помечаем флаг (асинхронно-безопасно) ======
static void alarm_handler(int sig){
    (void)sig;                   // не используем параметр
    timeout_occurred = 1;        // никаких printf/stdlib — только атомарная запись флага
}

// ====== Утилита: обрезка пробелов по краям (in-place) ======
static void trim(char **pb, char **pe){
    char *b=*pb, *e=*pe;
    while (b<e && isspace((unsigned char)*b)) b++;      // сдвигаем начало до первого непробела
    while (e>b && isspace((unsigned char)e[-1])) e--;   // сдвигаем конец к последнему непробелу
    *pb=b; *pe=e;                                       // возвращаем обновлённые границы
}

// ====== Чтение числа С ТАЙМЕРОМ (только для первого ввода) ======
// Возврат: 1 — успех, 0 — EOF, -1 — некорректный ввод, -2 — таймаут
static int read_int_with_timeout(long min,long max,long *out){
    char buf[64];                             // буфер для строки ввода
    timeout_occurred = 0;                     // сброс флага перед запуском таймера
    alarm(TIMEOUT);                           // включаем однократный будильник
    char *res = fgets(buf, sizeof buf, stdin);// блокирующее чтение строки (прервётся SIGALRM)
    alarm(0);                                 // выключаем таймер сразу после возврата из fgets

    if (!res){                                // NULL: либо EOF, либо прерывание
        if (timeout_occurred) return -2;      // сработал таймер → сообщаем о таймауте
        return 0;                             // иначе — EOF/ошибка ввода
    }
    if (!strchr(buf,'\n')){                   // строка не влезла целиком
        int c;                                // дочищаем хвост до конца строки
        while ((c=getchar())!='\n' && c!=EOF){}
    }

    char *p=buf, *e=buf+strlen(buf);          // p — начало, e — конец (за последним символом)
    trim(&p,&e); *e='\0';                     // удаляем пробелы по краям и завершаем строку
    if (*p=='\0'){ fprintf(stderr,"Пустой ввод. Повторите.\n"); return -1; }

    errno=0; char *q=NULL;                    // парсим целое число с проверками
    long v=strtol(p,&q,10);
    if (errno==ERANGE){ fprintf(stderr,"Число вне диапазона long.\n"); return -1; }
    if (q==p || *q!='\0'){ fprintf(stderr,"Ожидалось целое число.\n"); return -1; }
    if (v<min || v>max){ fprintf(stderr,"Число должно быть от %ld до %ld.\n",min,max); return -1; }

    *out=v;                                   // успех
    return 1;
}

// ====== Чтение числа БЕЗ таймера (для всех последующих вводов) ======
static int read_int_safe(long min,long max,long *out){
    char buf[64];
    if (!fgets(buf, sizeof buf, stdin)) return 0;     // EOF
    if (!strchr(buf,'\n')){ int c; while ((c=getchar())!='\n' && c!=EOF){} } // дочистить хвост

    char *p=buf, *e=buf+strlen(buf);
    trim(&p,&e); *e='\0';
    if (*p=='\0'){ fprintf(stderr,"Пустой ввод. Повторите.\n"); return -1; }

    errno=0; char *q=NULL; long v=strtol(p,&q,10);
    if (errno==ERANGE){ fprintf(stderr,"Число вне диапазона long.\n"); return -1; }
    if (q==p || *q!='\0'){ fprintf(stderr,"Ожидалось целое число.\n"); return -1; }
    if (v<min || v>max){ fprintf(stderr,"Число должно быть от %ld до %ld.\n",min,max); return -1; }

    *out=v;
    return 1;
}

// ====== Печать всего файла напрямую из mmap (при таймауте) ======
static void print_entire_file(void){
    if (!g_map || g_size==0) return;        // пустой файл или нет отображения
    puts("\n\nВремя на ввод истекло! Выводим содержимое файла:");
    puts("_____ПОЛНОЕ СОДЕРЖИМОЕ ФАЙЛА_____");
    (void)fwrite(g_map, 1, g_size, stdout); // печатаем одним куском весь мэппинг
    puts("\n_____КОНЕЦ ФАЙЛА_____");
}

// ====== Построение индекса строк поверх отображения (mmap) ======
static int build_index_from_mmap(const unsigned char *base, size_t sz,
                                 LineInfo *lines, int *pcount){
    int count = 0;                           // сколько строк нашли
    int cur_len = 0;                         // текущая длина строки

    if (sz == 0){ *pcount = 0; return 0; }   // пустой файл

    lines[0].offset = 0;                     // первая строка начинается в нуле

    for (size_t i=0; i<sz; ++i){             // линейный проход по байтам файла
        unsigned char ch = base[i];          // читаем байт из mmap (возможен page fault)
        cur_len++;                           // наращиваем текущую длину
        if (ch == '\n'){                     // конец строки?
            if (count < MAX_LINES){          // не переполняем индекс
                lines[count].length = cur_len - 1; // без '\n'
                printf("Строка %d: смещение = %ld, длина = %d\n",
                       count+1, lines[count].offset, lines[count].length);
                count++;
                if (count < MAX_LINES){
                    lines[count].offset = (long)(i+1); // начало следующей строки
                } else {
                    fprintf(stderr,"Достигнут лимит %d строк. Остальные игнорируются.\n", MAX_LINES);
                }
            }
            cur_len = 0;                     // сбрасываем длину для новой строки
        }
    }

    // Если файл не оканчивался '\n' — добавим последнюю строку
    if (cur_len > 0 && count < MAX_LINES){
        lines[count].length = cur_len;
        printf("Строка %d: смещение = %ld, длина = %d\n",
               count+1, lines[count].offset, lines[count].length);
        count++;
    }

    *pcount = count;                         // вернём количество строк
    return 0;
}

int main(int argc, char *argv[]){
    int fd = -1;                             // файловый дескриптор
    struct stat st;                          // для fstat (метаданные файла)
    LineInfo lines[MAX_LINES];               // индекс строк
    int line_count = 0;                      // количество строк

    if (argc != 2){                          // проверяем аргументы
        fprintf(stderr,"Использование: %s <filename>\n", argv[0]);
        return 1;
    }

    fd = open(argv[1], O_RDONLY);            // открываем файл только для чтения
    if (fd == -1){ perror("Ошибка открытия файла"); return 1; }

    if (fstat(fd, &st) == -1){               // узнаём размер и тип
        perror("fstat");
        close(fd);
        return 1;
    }
    if (!S_ISREG(st.st_mode)){               // запрещаем отображать не-обычные файлы
        fprintf(stderr,"Файл не является обычным.\n");
        close(fd);
        return 1;
    }

    size_t fsize = (size_t)st.st_size;       // безопасно приводим размер файла
    const unsigned char *map = NULL;         // сюда придёт адрес отображения

    if (fsize > 0){                          // пустой файл нельзя отобразить размером 0
        void *addr = mmap(NULL,              // адрес выбирает ядро
                          fsize,             // размер отображения = размер файла
                          PROT_READ,         // только чтение
                          MAP_PRIVATE,       // приватное (COW); нам запись не нужна
                          fd,                // дескриптор файла
                          0);                // смещение в файле (с начала)
        if (addr == MAP_FAILED){
            perror("mmap");
            close(fd);
            return 1;
        }
        map = (const unsigned char*)addr;    // сохраняем как байтовый указатель
    } else {
        map = NULL;                          // пустой файл
    }

    g_map = map;                             // глобальные для печати при таймауте
    g_size = fsize;

    printf("Файл '%s' успешно открыт, размер: %zu байт\n", argv[1], fsize);

    // Строим индекс строк, сканируя байты прямо в mmap
    printf("\n=== ПОСТРОЕНИЕ ТАБЛИЦЫ СТРОК (mmap) ===\n");
    if (build_index_from_mmap(map, fsize, lines, &line_count) != 0){
        fprintf(stderr,"Не удалось построить индекс.\n");
        if (map) munmap((void*)map, fsize);
        close(fd);
        return 1;
    }

    printf("\nВсего строк в файле: %d\n", line_count);
    if (line_count == 0){                    // пустой файл — корректный выход
        puts("Файл пуст — выход.");
        if (map) munmap((void*)map, fsize);
        close(fd);
        return 0;
    }

    // Готовим обработчик SIGALRM: без SA_RESTART, чтобы fgets прерывался таймером
    struct sigaction sa;
    sa.sa_handler = alarm_handler;           // наш простой обработчик — только флаг
    sigemptyset(&sa.sa_mask);                // не блокируем доп. сигналы
    sa.sa_flags = 0;                         // без автоматического рестарта syscalls
    if (sigaction(SIGALRM, &sa, NULL) == -1){
        perror("sigaction");
        if (map) munmap((void*)map, fsize);
        close(fd);
        return 1;
    }

    // ---- ПЕРВЫЙ ВВОД (с таймером) ---------------------------------
    printf("\n=== Интерактивный режим ===\n");
    printf("У вас есть %d секунд, чтобы ввести номер строки (первый ввод).\n", TIMEOUT);
    printf("Введите номер строки (1-%d) или 0 для выхода:\n> ", line_count);
    fflush(stdout);

    long ln;                                 // сюда пишем номер строки
    for(;;){
        int rc = read_int_with_timeout(0, line_count, &ln);
        if (rc == -2 || timeout_occurred){   // истёк таймер: печатаем весь файл и выходим
            print_entire_file();
            if (map) munmap((void*)map, fsize);
            close(fd);
            return 0;
        } else if (rc == 0){                 // EOF (Ctrl+D/закрыт stdin)
            puts("\nВыход (EOF)");
            if (map) munmap((void*)map, fsize);
            close(fd);
            return 0;
        } else if (rc < 0){                  // некорректный ввод — предложим снова
            printf("> "); fflush(stdout);
            continue;
        }
        break;                               // получено валидное число
    }
    if (ln == 0){                            // пользователь выбрал выход
        puts("Выход из программы");
        if (map) munmap((void*)map, fsize);
        close(fd);
        return 0;
    }

    // ---- ДАЛЕЕ БЕЗ ТАЙМЕРА: пользователь может выбирать строки бесконечно ----
    for (;;){
        int index = (int)ln - 1;             // переводим 1..N → 0..N-1
        long offset = lines[index].offset;   // смещение нужной строки
        int  length = lines[index].length;   // длина строки без '\n'

        printf("\nСтрока %ld: смещение = %ld, длина = %d\n", ln, offset, length);
        printf("Содержимое: '");

        // Печатаем сразу из отображения; ограничиваемся MAX_LINE_LENGTH для демонстрации
        int to_print = (length < MAX_LINE_LENGTH) ? length : MAX_LINE_LENGTH;
        if (to_print > 0 && (size_t)offset + (size_t)to_print <= g_size){
            (void)fwrite(g_map + offset, 1, (size_t)to_print, stdout);
        }
        printf("%s'\n", (length > to_print) ? " ...[обрезано]" : "");

        // Следующий выбор — без таймера
        printf("\nВведите номер строки (1-%d) или 0 для выхода:\n> ", line_count);
        fflush(stdout);

        int rc2 = read_int_safe(0, line_count, &ln);
        if (rc2 == 0){ puts("\nВыход (EOF)"); break; }     // stdin закрыт
        if (rc2 < 0){ continue; }                          // мусор — спрашиваем снова
        if (ln == 0){ puts("Выход из программы"); break; } // явный выход
    }

    if (map) munmap((void*)map, fsize);      // снимаем отображение (возврат страниц ядру)
    close(fd);                               // закрываем файл (уменьшаем refcount в ядре)
    return 0;                                // нормальный выход
}
