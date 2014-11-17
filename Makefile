CC=gcc
CFLAGS=-g -Wall -Werror -pedantic
LDFLAGS=
USER=cse533-14
TMP=ODR server client
BINS=$(TMP:=_$(USER))

all: $(BINS)

ODR_%: ODR.o
	$(CC) $(LDFLAGS) -o $@ $<

server_%: server.o
	$(CC) $(LDFLAGS) -o $@ $<

client_%: client.o
	$(CC) $(LDFLAGS) -o $@ $<

%.o: %.c %.h common.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o $(BINS)

PHONY: all clean
SECONDARY: server.o client.o
