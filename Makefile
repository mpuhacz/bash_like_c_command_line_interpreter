CC=gcc
LD=gcc

CFLAGS=--std=c11
LDFLAGS=

SRCS=$(shell ls src/*.c)
TARGET=SOPshell

OBJS=$(addsuffix .o, $(SRCS:src/%=obj/%))

$(TARGET): $(OBJS) 
	$(LD) $(LDFLAGS) $(OBJS) -o $(TARGET)

obj/%.c.o: src/%.c .depend
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) .depend

include .depend

.depend: $(SRCS) 
	gcc -MM $(SRCS) | sed 's/^[^.]*/obj\/\0.c/' > .depend2
	diff .depend2 .depend >/dev/null 2>&1 && rm .depend2 || mv .depend2 .depend
