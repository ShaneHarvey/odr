CC = gcc
FLAGS = -g -O2 -Wall -Werror -std=gnu89 -DCOLOR
LIB = libapi.a
LIBS = $(LIB)

USER=cse533-14
TMP=ODR server client
BINS=$(TMP:=_$(USER))

TARGETS = $(LIB) $(BINS)

all: $(TARGETS)

$(LIB): api.o api.h
	ar -cvq $(LIB) $<

ODR_%: ODR.o $(LIB)
	$(CC) -o $@ $< $(LIBS)

server_%: server.o $(LIB)
	$(CC) -o $@ $< $(LIBS)

client_%: client.o $(LIB)
	$(CC) -o $@ $< $(LIBS)

%.o: %.c %.h common.h
	$(CC) $(FLAGS) -c $<

clean:
	rm -f *.o $(TARGETS)

PHONY: all clean
SECONDARY: server.o client.o
