TARGET	:= abuse
OBJS	:= $(TARGET:=.o)

CC		:= /usr/bin/gcc
CFLAGS	:= -g -pedantic -Wall -Wextra -I$(HOME)/local/include -I$(HOME)/src/nbd
LDFLAGS	:= -lz

.PHONY: all clean
all: $(TARGET)

$(TARGET): %: %.o
	$(CC) $(LDFLAGS) -o $@ $<

$(OBJS): %.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS)
