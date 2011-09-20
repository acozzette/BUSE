TARGET	:= abuse_example
OBJS	:= $(TARGET:=.o) abuse.o

C		:= /usr/bin/gcc
CFLAGS	:= -g -pedantic -Wall -Wextra -std=c99 -I$(HOME)/local/include -I$(HOME)/src/nbd
LDFLAGS	:=

.PHONY: all clean
all: $(TARGET)

$(TARGET): $(OBJS) abuse.h
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS)
