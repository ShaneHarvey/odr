CC = gcc
# -O2
FLAGS = -Wall -Werror -std=gnu89 -DCOLOR
LIB = libapi.a
LIBS = $(LIB)

USER=cse533-14
TMP=ODR server client
BINS=$(TMP:=_$(USER))

TARGETS = $(LIB) $(BINS)

all: $(TARGETS)

debug: FLAGS += -DDEBUG -g
debug: $(TARGETS)

$(LIB): api.o api.h
	ar -cvq $(LIB) $<

ODR_%: ODR.o
	$(CC) -o $@ $< $(LIBS)

server_%: server.o
	$(CC) -o $@ $< $(LIBS)

client_%: client.o
	$(CC) -o $@ $< $(LIBS)

%.o: %.c %.h common.h
	$(CC) $(FLAGS) -c $<

clean:
	rm -f *.o $(TARGETS)

PHONY: all clean
SECONDARY: server.o client.o
