HOSTCC:=$(CC)
CC:=$(CROSS_COMPILE)$(HOSTCC)
CFLAGS:=-std=c99 -Wall -O2 $(CFLAGS) -I extra/include

HOSTLD:=$(LD)
LD:=$(CROSS_COMPILE)$(HOSTLD)
LDFLAGS:=-L extra/lib

TEMPER_OBJS:=comm.o

all:	temper

%.o:	%.c
	$(CC) -c $(CFLAGS) -DUNIT_TEST -o $@ $^

temper:		$(TEMPER_OBJS) temper.o
	$(CC) $(LDFLAGS) -o $@ $^ -lusb

clean:		
	rm -f temper *.o

rules-install:			# must be superuser to do this
	cp 99-tempsensor.rules /etc/udev/rules.d

