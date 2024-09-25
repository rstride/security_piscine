CC=gcc
CFLAGS=-I./includes -Wall -Wextra -O2
LDFLAGS=-lssl -lcrypto

SRC=src/main.c src/encryption.c src/decryption.c src/utils.c
OBJ=$(SRC:.c=.o)

all: stockholm

stockholm: $(OBJ)
	$(CC) -o stockholm $(OBJ) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) stockholm

re: clean all

.PHONY: all clean re