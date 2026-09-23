# Toolchain setup
CC = gcc
CFLAGS = -O0 -g -Wall
KAORUC = kaoru-compiler

# Build outputs
TARGET = run_benchmark
OBJS = benchmark.o test_scope.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

benchmark.o: benchmark.c
	$(CC) $(CFLAGS) -c $< -o $@

test_scope.o: test_scope.ml
	# Compile Marlov down to x64 object file
	$(KAORUC) --target=x64 -c $< -o $@

clean:
	rm -f *.o $(TARGET)