#include <sys/types.h> // СДАЛ
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
extern char *tzname[];

int main()
{
    time_t now;
    struct tm *sp;

    //временная зона в Калифорнии (только PST)
    setenv("TZ", "PST8", 1);
    tzset();

    (void) time(&now);

    sp = localtime(&now);
    printf("California time (PST): %d/%d/%02d %d:%02d %s\n",
        sp->tm_mon + 1, sp->tm_mday,
        sp->tm_year%100, sp->tm_hour,
        sp->tm_min, tzname[sp->tm_isdst]);
    
    exit(0);
}