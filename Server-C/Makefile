# Compiler
CC = gcc

# OpenSSL paths (Ubuntu default)
LDFLAGS = -L/usr/lib/x86_64-linux-gnu
CPPFLAGS = -I/usr/include -Wall

# Source files
SRCS = server.c

# Object files
OBJS = $(SRCS:.c=.o)

# Target executable
TARGET = server

# Default target
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LDLIBS) -lssl -lcrypto

# Compiling the source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleaning up
clean:
	rm -f $(OBJS) $(TARGET)

