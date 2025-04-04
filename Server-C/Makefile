# Compiler and flags
CC = gcc
CFLAGS = -Wall -g
LIBS = -lcjson -lssl -lcrypto
LDFLAGS =

INCLUDES = -I/usr/include/json-c -I/usr/include/openssl

SRC = server.c base64.c  

OUT = server

# Default target
all: $(OUT)

# Compile and link the program
$(OUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) $(INCLUDES) -o $(OUT) $(LIBS)

# Clean up object files and the executable
clean:
	rm -f $(OUT) *.o

# Install dependencies (for systems where the json-c and OpenSSL libraries aren't installed)
install-dependencies:
	@echo "Installing json-c and OpenSSL dependencies..."
	@sudo apt-get install -y libjson-c-dev libssl-dev  # For Ubuntu/Debian systems
	# If you use other distros like CentOS/Fedora, uncomment below:
	# @sudo yum install -y json-c-devel openssl-devel
	# For macOS (Homebrew)
	# @brew install json-c openssl
