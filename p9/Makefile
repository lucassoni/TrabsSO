CC     = gcc
CFLAGS = -Wall -g
DEBUG_FLAGS = -DDEBUG

PROG = pingpong-sleep
OBJS = ppos_core.o queue.o

.PHONY: clean purge all debug

all: $(PROG)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROG) : % :  $(OBJS) %.o
	$(CC) -o $@ $^

clean:
	@rm -f *.o

purge:   
	@rm -f $(PROG)
