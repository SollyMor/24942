#!/bin/bash

echo "Быстрое тестирование string_list..."

g++ -o string_list string_list.cpp || exit 1

echo "ТЕСТ 1: Основной"
echo -e "one\ntwo\nthree\n." | ./string_list

echo "ТЕСТ 2: Пустые строки"
echo -e "first\n\nlast\n." | ./string_list

echo "ТЕСТ 3: Длинная строка"
echo "This is a very long line that tests the buffer" | ./string_list

echo "Тестирование завершено"
rm -f string_list