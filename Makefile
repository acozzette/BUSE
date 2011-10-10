TARGET	:= busexmp
OBJS	:= $(TARGET:=.o) buse.o

C		:= /usr/bin/gcc
CFLAGS	:= -g -pedantic -Wall -Wextra -std=c99 -I$(HOME)/local/include -I$(HOME)/src/nbd
LDFLAGS	:=

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJS): %.o: %.c buse.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS)
