CFLAGS=-O2 -Wall -Wextra -g3

all: space-management.so

space-management.so: space-management.c Makefile
	$(CC) $(CFLAGS) -shared -fPIC space-management.c -ldl -o space-management.so
clean:
	$(RM) space-management.so

.PHONY: test
test:
	$(PWD)/test.sh
	env SPACE_MANAGEMENT_RETRIES=0 $(PWD)/test.sh && exit 1 || exit 0
