# # Compiler
CC = gcc

# OpenSSL paths (Ubuntu default)
LDFLAGS = -L/usr/lib/x86_64-linux-gnu
CPPFLAGS = -I/usr/include -Wall -I./include  # Add the path to base64.h if it's in 'include/'

# Source files
SRCS = server.c base64.c  # Add base64.c if it exists

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
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

# Cleaning up
clean:
	rm -f $(OBJS) $(TARGET)

