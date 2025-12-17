#!/bin/bash

echo "=== Тестирование process_info ==="
echo "Система: $(uname -s)"
echo "Дата: $(date)"
echo ""

if [ ! -f "./process_info" ]; then
    echo "ОШИБКА: process_info не найдена!"
    echo "Скомпилируйте: gcc -o process_info process_info.c"
    exit 1
fi

echo "1. БАЗОВЫЕ ОПЦИИ:"
./process_info -i
./process_info -p
./process_info -u
./process_info -c
./process_info -d

echo "2. ИЗМЕНЕНИЕ НАСТРОЕК:"
./process_info -U512 -u
./process_info -C1024 -c
./process_info -VMY_TEST=hello -v | grep MY_TEST

echo "3. ОБРАБОТКА СПРАВА НАЛЕВО:"
./process_info -i -p -d

echo "4. МНОГОКРАТНЫЕ ОПЦИИ:"
./process_info -i -p -i -v | head -15

echo "5. ОБРАБОТКА ОШИБОК:"
./process_info -x
./process_info -Uinvalid
./process_info -Cinvalid
./process_info -Vinvalid

echo "6. СПЕЦИАЛЬНЫЕ ТЕСТЫ:"
./process_info -s -p
./process_info -i -u -c -d | head -8

echo "=== ТЕСТИРОВАНИЕ ЗАВЕРШЕНО ==="