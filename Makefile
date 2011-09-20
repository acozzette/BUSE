TARGET	:= abuse_example
OBJS	:= $(TARGET:=.o)

C		:= /usr/bin/gcc
CFLAGS	:= -g -pedantic -Wall -Wextra -std=c99 -I$(HOME)/local/include -I$(HOME)/src/nbd
LDFLAGS	:= -lz

.PHONY: all clean
all: $(TARGET) abuse.h

$(TARGET): %: %.o
	$(CC) $(LDFLAGS) -o $@ $<

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS)
