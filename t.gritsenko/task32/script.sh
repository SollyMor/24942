#!/bin/bash

SOCKET=./socket

echo "=== Очистка старого сокета ==="
rm -f $SOCKET

echo "=== Запуск сервера ==="
./server &
SERVER_PID=$!

# даём серверу подняться
sleep 1

echo "=== Запуск клиентов ==="

# Клиент 1: шлёт X по одному символу
(
  while true; do
    printf "xxx"
    sleep 0.02
  done
) | ./client &

CLIENT1_PID=$!

# Клиент 2: шлёт Y по одному символу
(
  while true; do
    printf "yyy"
    sleep 0.02
  done
) | ./client &

CLIENT2_PID=$!

# Дать поработать
sleep 5

echo
echo "=== Остановка ==="

kill $CLIENT1_PID $CLIENT2_PID 2>/dev/null
kill $SERVER_PID 2>/dev/null

wait 2>/dev/null

echo "=== Готово ==="
