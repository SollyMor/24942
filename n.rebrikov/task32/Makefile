CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -D_XOPEN_SOURCE=600
LDFLAGS = -lsocket -lnsl -lrt  # -lrt ДЛЯ AIO НА SOLARIS!
TARGETS = server_aio client_aio test_client_aio demo_aio

all: $(TARGETS)

# Асинхронный сервер
server_aio: server_aio.c
	$(CC) $(CFLAGS) -o server_aio server_aio.c $(LDFLAGS)

# Клиент для асинхронного сервера
client_aio: client_aio.c
	$(CC) $(CFLAGS) -o client_aio client_aio.c $(LDFLAGS)

# Тестовый клиент для демонстрации смешения
test_client_aio: test_client_aio.c
	$(CC) $(CFLAGS) -o test_client_aio test_client_aio.c $(LDFLAGS)

# Скрипт демонстрации
demo_aio: server_aio test_client_aio
	@chmod +x demo_aio.sh
	@echo "Use: ./demo_aio.sh to run aio demonstration"

# Очистка
clean:
	rm -f $(TARGETS) server_aio client_aio test_client_aio socket32 *.o
	rm -f aio_server aio_client

# Запуск асинхронного сервера
run_aio_server: server_aio
	./server_aio

# Демонстрация асинхронной работы
demo_aio_mix: server_aio test_client_aio
	@echo "=== Starting ASYNC demonstration ==="
	@./server_aio &
	@sleep 2
	@echo "Starting 3 async clients..."
	@./test_client_aio 1 &
	@./test_client_aio 2 &
	@./test_client_aio 3 &
	@echo "Wait for clients to finish..."
	@wait
	@sleep 3
	@pkill -f "./server_aio" || true
	@echo "=== Async demonstration finished ==="

# Тест с множеством клиентов
stress_test: server_aio test_client_aio
	@echo "=== Stress test with 5 async clients ==="
	@./server_aio &
	@sleep 2
	@for i in 1 2 3 4 5; do \
		./test_client_aio $$i & \
		sleep 0.2; \
	done
	@echo "All async clients started..."
	@wait
	@sleep 3
	@pkill -f "./server_aio" || true
	@echo "=== Stress test finished ==="

.PHONY: all clean run_aio_server demo_aio demo_aio_mix stress_test
