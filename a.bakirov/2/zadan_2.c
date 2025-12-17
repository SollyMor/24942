#include <stdio.h>
#include <time.h>
#include <stdlib.h>

extern char *tzname[];

int main() {

    time_t now; // создаём объект для получения значения из функции time

    struct tm *sp; // инициализируем структуру 

    putenv("TZ=America/Los_Angeles");

    now = time(NULL); // столько-то секунд прошло с 1970 года
    printf("%s", ctime(&now));

    sp = localtime(&now);
    printf("%d/%d/%02d %d:%02d %s\n",
           sp->tm_mon + 1, sp->tm_mday,
           sp->tm_year - 100, sp->tm_hour,
           sp->tm_min, tzname[sp->tm_isdst]);

    return 0;
}