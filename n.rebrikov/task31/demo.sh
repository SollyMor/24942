#!/bin/bash
# demo.sh - Demonstration of message mixing

echo "========================================"
echo "  DEMONSTRATION OF MESSAGE MIXING"
echo "  Task 31: Unix Socket with poll()"
echo "========================================"

# Компиляция
echo -e "\n1. Compiling..."
make clean > /dev/null
make server test_client > /dev/null

# Удаляем старый сокет
rm -f socket31

echo -e "\n2. Starting server in background..."
./server &
SERVER_PID=$!
sleep 2

echo -e "\n3. Starting 3 clients SIMULTANEOUSLY..."
echo "   Each client will send 15 messages with random delays"
echo "   Watch how messages get mixed in server output!"
echo "   ------------------------------------------"

# Запускаем клиентов с небольшими смещениями по времени
./test_client 1 &
CLIENT1_PID=$!
sleep 0.05  # 50ms смещение

./test_client 2 &
CLIENT2_PID=$!
sleep 0.05  # 50ms смещение

./test_client 3 &
CLIENT3_PID=$!
sleep 0.05  # 50ms смещение

echo -e "\nClients started with PIDs: $CLIENT1_PID, $CLIENT2_PID, $CLIENT3_PID"
echo "Waiting for clients to finish..."

# Ждем завершения клиентов
wait $CLIENT1_PID $CLIENT2_PID $CLIENT3_PID

echo -e "\n4. All clients finished."
echo "   Giving server time to process remaining data..."
sleep 3

echo -e "\n5. Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "\n========================================"
echo "  DEMONSTRATION COMPLETE!"
echo "  Check the output above to see mixed messages."
echo "  Messages from different clients should be interleaved."
echo "========================================"

# Очистка
make clean > /dev/null 2>&1
