CC = gcc
CFLAGS = -Wall -Werror -std=gnu89 -DCOLOR
LIB = libapi.a
LIBS = $(LIB)

USER=cse533-14
TMP=ODR server client
BINS=$(TMP:=_$(USER))

TARGETS = $(LIB) $(BINS)

all: $(TARGETS)

debug: CFLAGS += -DDEBUG -g
debug: $(TARGETS)

$(LIB): api.o api.h
	ar -cvq $(LIB) $<

ODR_%: ODR.o common.o get_hw_addrs.o
	$(CC) -o $@ $^ -lrt

server_%: server.o common.o $(LIB)
	$(CC) -o $@ $< common.o $(LIBS)

client_%: client.o $(LIB)
	$(CC) -o $@ $< $(LIBS)

%.o: %.c %.h common.h
	$(CC) $(CFLAGS) -c $<

prhwaddrs: prhwaddrs.c get_hw_addrs.o
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f *.o $(TARGETS) prhwaddrs

PHONY: all clean
SECONDARY: server.o client.o ODR.o common.o get_hw_addrs.o
