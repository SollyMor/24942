#!/bin/bash

echo "========================================"
echo "  ASYNC SERVER DEMONSTRATION"
echo "  Task 32: Non-blocking I/O"
echo "========================================"

# Удаляем старый сокет
rm -f socket32

echo -e "\n1. Starting async server..."
./server_aio &
SERVER_PID=$!
sleep 3

echo -e "\n2. Starting 3 clients with different delays..."
echo "   Watch how messages get mixed in server output!"
echo "   ------------------------------------------"

# Запускаем клиентов почти одновременно
for i in 1 2 3; do
    echo "Starting client $i..."
    ./test_client_aio $i &
    sleep 0.1
done

echo -e "\nWaiting for clients to finish..."
wait

echo -e "\n3. Giving server time to process..."
sleep 3

echo -e "\n4. Stopping server..."
kill $SERVER_PID 2>/dev/null
wait $SERVER_PID 2>/dev/null

echo -e "\n========================================"
echo "  DEMONSTRATION COMPLETE!"
echo "  Server used non-blocking I/O with select()"
echo "  Messages were processed asynchronously"
echo "========================================"
