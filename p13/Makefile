CC     = gcc
CFLAGS = -Wall -g
CLIBS = -lm -lrt
DEBUG_FLAGS = -DDEBUG

PROG = pingpong-disco2 pingpong-disco1
OBJS = ppos_core.o queue.o ppos_disk.o disk.c

.PHONY: clean purge all

all: $(PROG)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROG) : % :  $(OBJS) %.o
	$(CC) -o $@ $^ $(CLIBS)

clean:
	@rm -f *.o

purge:   
	@rm -f $(PROG)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all