HOSTCC:=$(CC)
CC:=$(CROSS_COMPILE)$(HOSTCC)

HOSTLD:=$(LD)
LD:=$(CROSS_COMPILE)$(HOSTLD)

CFLAGS:=-std=c99 -Wall -O2 $(CFLAGS) -I extra/include

LDFLAGS:=-L extra/lib

all:	temper

temper.o:	temper.c
	$(CC) -c $(CFLAGS) -DUNIT_TEST -o $@ $^

temper:		temper.o
	$(CC) $(LDFLAGS) -o $@ $^ -lusb

clean:		
	rm -f temper *.o

rules-install:			# must be superuser to do this
	cp 99-tempsensor.rules /etc/udev/rules.d

