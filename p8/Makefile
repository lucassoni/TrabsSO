CC     = gcc
CFLAGS = -Wall -g

PROG = pingpong-join
OBJS = ppos_core.o queue.o

.PHONY: clean purge all debug

all: $(PROG)

%.o: %.c
	$(CC) -c $(CFLAGS) $<

$(PROG) : % :  $(OBJS) %.o
	$(CC) -o $@ $^

clean:
	@rm -f *.o

purge:   
	@rm -f $(PROG)
