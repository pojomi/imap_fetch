CC = gcc
CFLAGS = -Wall
LIBS = -lssl -lcrypto

imap_fetch: ./src/imap_fetch.c ./src/utils.c ./src/decoders.c
	$(CC) $(CFLAGS) ./src/imap_fetch.c ./src/utils.c ./src/decoders.c -o imap_fetch $(LIBS)

clean:
	rm -f imap_fetch