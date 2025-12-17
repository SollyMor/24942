/*

 * ИДЕЯ: 
    1. Захватываем (блокируем) весь файл эксклюхивно с помощью fcntl(F_SETLKM)
    2. Пока файл заблокирован, запускаем редактор(через system())
    3. После выхода из редактора снимаем блокировку

 * Advisory (допустимый) режим
    - работает "По договоренности": только те процессы, которые тоже используют fcntl(),
    видят блокировку и ждут
    - если другой процесс (например, редактор напрямую или cp/cat) просто открывает файл, 
    ядро ему это не запрещает

 * Mandatory (обязательный) режим: 
    - включается режимом `chmod +l file` 
    - ядро само запрещает доступ другим процессам к файлу при активном захвате 
    - для работы нужно, чтобы файл был на локальной файловой системе(не NFS)
 */

#define _FILE_OFFSET_BITS 64 // чтобы open/fcntl работали с большими файлами 

#include <stdio.h>      // fprintf, perror
#include <stdlib.h>     // getenv, system, exit, malloc, free
#include <string.h>     // strlen, strchr, memcpy
#include <errno.h>      // errno
#include <fcntl.h>      // open, fcntl, struct flock
#include <unistd.h>     // close, access
#include <sys/stat.h>   // open flags
#include <limits.h>     // PATH_MAX

/*-------------------------------------------------------------------------
 * Вспомогательная функция: экранируем имя файла для передачи в shell.
 * Например, путь my'file.txt преврашается в 'my'\''file.txt'
 * чтобы system("/bin/sh -c ...") не сломал команду на кавычке.
 * Возвращает malloc-нутую строку, которую надо потом free()
 *-------------------------------------------------------------------------*/
static char *shell_single_quote(const char *path) {
    size_t n = strlen(path);
    /* В худшем случае каждый символ ' превращается в четырехсимвольную последовательность '\'' */
    size_t worst = 2 + n * 4 + 1; // начальная и конечная кавычки + worst + '\0'
    char *out = (char *)malloc(worst);
    if (!out) return NULL;

    char *p = out;
    *p++ = '\'';   //открываем одинарную кавычку
    for (size_t i = 0; i < n; ++i) {
        if (path[i] == '\'') {
            /* закрыть ', добавить \', открыть ' снова */
            memcpy(p, "'\\''", 4);
            p += 4;
        } else {
            *p++ = path[i];
        }
    }
    *p++ = '\'';   // закрываем кавычку
    *p = '\0';
    return out;
}

/* Печать короткой справки 
 * usage() - печатает короткую справку
*/
static void usage(const char *prog) {
    fprintf(stderr,
        "Usage: %s <path/to/file>\n"
        "Env:\n"
        "  EDITOR  — имя редактора (по умолчанию: vi)\n"
        "  MLOCK=1 — вы хотите режим mandatory (включите перед этим `chmod +l <file>`)\n",
        prog);
}

/* Установить эксклюзивную блокировку на весь файл (блокирующе). 
 * lock_whole_file() - установить эксклюзивный захват на весь файл
 *  F_WRLCK - write lock - эсклюзивный блокировка 
 *  F_SET
*/
static int lock_whole_file(int fd) {
    struct flock lk;
    lk.l_type   = F_WRLCK;   // эксклюзивный захват «на запись»
    lk.l_whence = SEEK_SET;  // от начала
    lk.l_start  = 0;         // с нулевого смещения
    lk.l_len    = 0;         // до конца файла (0 == "до EOF", то есть «весь файл»)
    lk.l_pid    = 0;         // ядро заполнит при F_GETLK; здесь не требуется

    if (fcntl(fd, F_SETLKW, &lk) == -1) { // «W» — ждать, пока не получится
        return -1;
    }
    return 0;
}

/* Снять блокировку с всего файла */
static int unlock_whole_file(int fd) {
    struct flock lk;
    lk.l_type   = F_UNLCK;
    lk.l_whence = SEEK_SET;
    lk.l_start  = 0;
    lk.l_len    = 0;
    lk.l_pid    = 0;

    if (fcntl(fd, F_SETLK, &lk) == -1) {
        return -1;
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        usage(argv[0]);
        return 2;
    }
    const char *filepath = argv[1];

    /* Небольшие подсказки пользователю: доступность файла и режим mandatory */
    if (getenv("MLOCK")) {
        /* Напоминаем: для mandatory в Solaris нужно включить атрибут `+l` у файла,
         * и лучше хранить файл в локальной ФС (не NFS).
         */
        fprintf(stderr,
            "[info] Mandatory lock requested (MLOCK=1).\n"
            "       Make sure you ran:  chmod +l %s\n"
            "       Also ensure the file is on a local FS (not NFS).\n",
            filepath);
    }

    /* Открываем файл для чтения/записи (если нужен только «на запись», всё равно O_RDWR — понятнее для редактора) */
    int fd = open(filepath, O_RDWR);
    if (fd == -1) {
        perror("open");
        return 1;
    }

    /* Ставим эксклюзивную блокировку на весь файл, блокирующе ждём, если занят */
    if (lock_whole_file(fd) == -1) {
        perror("fcntl(F_SETLKW, F_WRLCK)");
        close(fd);
        return 1;
    }

    /* Готовим команду запуска редактора:
     * Берём $EDITOR, иначе vi; передаём имя файла в одинарных кавычках (с экранированием).
     */
    const char *editor = getenv("EDITOR");
    if (!editor || !*editor) editor = "vi";

    char *qpath = shell_single_quote(filepath);
    if (!qpath) {
        fprintf(stderr, "memory allocation failure\n");
        unlock_whole_file(fd);
        close(fd);
        return 1;
    }

    /* Собираем финальную команду вида:  <editor> 'имя_файла' */
    size_t cmd_len = strlen(editor) + 1 + strlen(qpath) + 1;
    char *cmd = (char *)malloc(cmd_len);
    if (!cmd) {
        fprintf(stderr, "memory allocation failure\n");
        free(qpath);
        unlock_whole_file(fd);
        close(fd);
        return 1;
    }
    /* Формат: "<editor> <qpath>" */
    snprintf(cmd, cmd_len, "%s %s", editor, qpath);

    /* Запуск редактора. Здесь system(3) создаёт дочерний процесс, выполняет команду в /bin/sh -c
     * и ждёт завершения редактора, возвращая его код статуса.
     */
    int rc = system(cmd);

    /* Чистим временные ресурсы */
    free(cmd);
    free(qpath);

    /* Снимаем блокировку (важно сделать в любом случае, даже если редактор упал) */
    if (unlock_whole_file(fd) == -1) {
        perror("fcntl(F_SETLK, F_UNLCK)");
        close(fd);
        return 1;
    }

    /* Закрываем файл */
    if (close(fd) == -1) {
        perror("close");
        return 1;
    }

    /* Пробрасываем код возврата редактора (по возможности) */
    if (rc == -1) {
        perror("system");
        return 1;
    }
    /* system() возвращает код в формате wait(2); упростим: 0 — успех, иначе 1 */
    return (rc == 0) ? 0 : 1;
}
