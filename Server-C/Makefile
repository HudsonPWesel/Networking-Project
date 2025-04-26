# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LIBS = -lcjson -lssl -lcrypto -lmysqlclient
LDFLAGS =

INCLUDES = -I/usr/include/json-c -I/usr/include/openssl -I/usr/include/mysql

SRC = server.c crypto.c auth.c database.c main.c websocket.c game.c

OUT = server

# Default target
all: $(OUT)

# Compile and link the program
$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(INCLUDES) -o $(OUT) $(LIBS)

# Clean up object files and the executable
clean:
	rm -f $(OUT) *.o

# Install dependencies
install-dependencies:
	@echo "Installing json-c, OpenSSL, and MySQL client libraries..."
	@sudo apt-get install -y libjson-c-dev libssl-dev default-libmysqlclient-dev
