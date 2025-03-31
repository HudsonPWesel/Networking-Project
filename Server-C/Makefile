# Compiler
CC = gcc


# Linker flags

LDFLAGS="-L /opt/homebrew/opt/openssl@3.4" \
CPPFLAGS="-I /opt/homebrew/opt/openssl@3.4/include" \

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
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS) $(LDLIBS)

# Compiling the source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleaning up
clean:
	rm -f $(OBJS) $(TARGET)
