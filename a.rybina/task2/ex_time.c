#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>

extern char *tzname[]; //stores the names of the time zones

int main(){
    time_t now;

    setenv("TZ", "PST8", 1);
    tzset();

    (void) time(&now); //get current time

    printf("Current time in California:\n");
    printf("%s", ctime( &now ) );

    return 0;
}