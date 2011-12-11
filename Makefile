# Makefile for rinput
# Heiher <admin@heiher.info>

CC=gcc
CFLAGS=
LDFLAGS=

all : rinputd rinput

rinputd : rinputd.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

rinput : rinput.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

clean : 
	@rm -f rinput rinputd

