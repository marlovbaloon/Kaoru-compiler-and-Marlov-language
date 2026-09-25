# Toolchain setup
CC = gcc
CFLAGS = -O0 -g -Wall
KAORUC = ./kaoru-compiler

# Build outputs
TARGET = run_benchmark
OBJS = benchmark.o test_scope.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

benchmark.o: benchmark.c
	$(CC) $(CFLAGS) -c $< -o $@

test_scope.o: test_scope.ml
	# 1
	$(KAORUC) --target=x64 -c $< -o test_scope.s
	# 2
	$(CC) -c test_scope.s -o $@

clean:
	rm -f *.o *.s $(TARGET)