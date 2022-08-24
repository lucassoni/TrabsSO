CC     = gcc
CFLAGS = -Wall
CLIBS = -lm
DEBUG_FLAGS = -DDEBUG

PROG = pingpong-mqueue
OBJS = ppos_core.o queue.o

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