CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lsocket -lnsl
TARGETS = server client test_client demo

all: $(TARGETS)

# Сервер
server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

# Клиент для ручного ввода
client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

# Клиент для автоматического тестирования (смешение)
test_client: test_client.c
	$(CC) $(CFLAGS) -o test_client test_client.c $(LDFLAGS)

# Демо клиент (простой)
demo_client: demo_client.c
	$(CC) $(CFLAGS) -o demo_client demo_client.c $(LDFLAGS)

# Тестовый скрипт
demo: server test_client
	@chmod +x demo.sh
	@echo "Use: ./demo.sh to run demonstration"

# Очистка
clean:
	rm -f $(TARGETS) server client test_client demo_client socket31
	rm -f *.o

# Запуск сервера
run_server: server
	./server

# Быстрая демонстрация смешения
demo_mix: server test_client
	@echo "=== Starting mixing demonstration ==="
	@./server &
	@sleep 2
	@echo "Starting 3 test clients simultaneously..."
	@./test_client 1 &
	@./test_client 2 &
	@./test_client 3 &
	@echo "Wait for clients to finish..."
	@wait
	@sleep 2
	@pkill -f "./server" || true
	@echo "=== Demonstration finished ==="

# Тест с 5 клиентами
test_heavy: server test_client
	@echo "=== Heavy test with 5 clients ==="
	@./server &
	@sleep 2
	@for i in 1 2 3 4 5; do \
		./test_client $$i & \
		sleep 0.1; \
	done
	@echo "All clients started..."
	@wait
	@sleep 2
	@pkill -f "./server" || true
	@echo "=== Test finished ==="

.PHONY: all clean run_server demo demo_mix test_heavy
