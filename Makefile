TARGET		:= busexmp loopback
LIBOBJS 	:= buse.o
OBJS		:= $(TARGET:=.o) $(LIBOBJS)
STATIC_LIB	:= libbuse.a

CC		:= /usr/bin/gcc
override CFLAGS += -g -pedantic -Wall -Wextra -std=c99
LDFLAGS		:= -L. -lbuse

.PHONY: all clean test
all: $(TARGET)

$(TARGET): %: %.o $(STATIC_LIB)
	$(CC) -o $@ $< $(LDFLAGS)

$(TARGET:=.o): %.o: %.c buse.h
	$(CC) $(CFLAGS) -o $@ -c $<

$(STATIC_LIB): $(LIBOBJS)
	ar rcu $(STATIC_LIB) $(LIBOBJS)

$(LIBOBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

test: $(TARGET)
	PATH=$(PWD):$$PATH sudo test/busexmp.sh
	PATH=$(PWD):$$PATH sudo test/signal_termination.sh

clean:
	rm -f $(TARGET) $(OBJS) $(STATIC_LIB)
