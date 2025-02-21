CC=gcc
CFLAGS=-I. -v 
deps = base64.h sha1.h
OBJ = server.o base64.o sha1.o

# for any .o file, compile it from the corresponding .c file, while also depending on the headers in $(DEPS)"
# $(DEPS) tell The dependency tells Make "rebuild this .o file if base64.h changesw"
# $@ -> Target (server.o)
# $<  $< ALWAYS refers to the first prerequisite listed after the colon : (server.c)

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS) # expanded version gcc -c -o server.o server.c 

	#   creates the final executable by linking all the object files together
	# 	make with no arguments, it tries to build the first target in the Makefile "default goal"
	# 	So before Make can build server, it needs both server.o and base64.o 
	# 	It finds the pattern rule %.o: %.c $(DEPS) that matches above and uses that rule to build the .o file

server: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS) # expanded gcc -o server server.o base64.o
clean:
	rm -f $(OBJ) server
