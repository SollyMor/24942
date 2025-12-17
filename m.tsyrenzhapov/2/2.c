#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>

int main()
{
    time_t now;
    struct tm *sp;
    
    // Получаем текущее время в UTC
    (void) time(&now);
    
    // Принудительно вычисляем время в PST (UTC-8)
    now = now - 8 * 3600;  // Вычитаем 8 часов
    
    setenv("TZ", "UTC", 1); // Говорим системе: "используй UTC"
    tzset(); // применение новых настроек
    
    // Преобразуем время
    sp = localtime(&now); //тут появились минуты секунды часы
    
    printf("Текущее время в Калифорнии (PST):\n");
    printf("%d/%d/%02d %d:%02d PST\n", sp->tm_mon + 1, sp->tm_mday,sp->tm_year % 100, sp->tm_hour, sp->tm_min);
}