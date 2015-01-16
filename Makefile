CFLAGS=-O2 -Wall -Wextra -g3

all: space-management.so

space-management.so: space-management.c Makefile
	$(CC) $(CFLAGS) -shared -fPIC space-management.c -ldl -o space-management.so
clean:
	$(RM) space-management.so
