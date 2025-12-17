#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char time_buffer[256];
    time_t current_time;
    struct tm *utc_time;
    
    printf("=== Время в Калифорнии (Pacific Time) ===\n");
    
    if (time(&current_time) == (time_t)-1) {
        perror("Ошибка получения времени");
        return 1;
    }
    
    utc_time = gmtime(&current_time);
    if (utc_time == NULL) {
        perror("Ошибка преобразования UTC времени");
        return 1;
    }
    
    struct tm california_time = *utc_time;
    
    int is_daylight_saving = (california_time.tm_mon >= 2 && california_time.tm_mon <= 10);

    int hour_offset = is_daylight_saving ? -7 : -8;
    california_time.tm_hour += hour_offset;
    
    mktime(&california_time);

    if (strftime(time_buffer, sizeof(time_buffer), 
                 "%Y-%m-%d %H:%M:%S", &california_time) == 0) {
        fprintf(stderr, "Ошибка форматирования времени\n");
        return 1;
    }
    
    printf("Текущее время в Калифорнии: %s %s\n", time_buffer, 
           is_daylight_saving ? "PDT (UTC-7)" : "PST (UTC-8)");
    
    printf("Соответствие UTC: если UTC %02d часов, то в Калифорнии %02d часов\n",
           utc_time->tm_hour, california_time.tm_hour);
    
    printf("Разница с UTC: UTC%d\n", hour_offset);
    
    if (strftime(time_buffer, sizeof(time_buffer), 
                 "%Y-%m-%d %H:%M:%S UTC", utc_time) == 0) {
        fprintf(stderr, "Ошибка форматирования UTC времени\n");
        return 1;
    }
    printf("Текущее время UTC: %s\n", time_buffer);
    
    return 0;
}

//gcc -o california_time california_time.c
// ./california_time