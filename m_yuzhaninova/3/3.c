#include <stdio.h>
#include <unistd.h>

void check_file(const char *fname)
{
    FILE *file = fopen(fname, "r");
    if (file)
    {
        printf("Файл открыт успешно\n\n");
        fclose(file);
    }
    else
    {
        perror("Ошибка при открытии файла");
    }
}

void show_ids()
{
    printf("Реальный UID: %d\n", getuid());
    printf("Эффективный UID: %d\n\n", geteuid());
}

int main()
{
    const char *fname = "text.txt";

    show_ids();
    check_file(fname);

    if (setuid(getuid()) != 0)
    {
        perror("Ошибка setuid");
    }

    show_ids();
    check_file(fname);

    return 0;
}
