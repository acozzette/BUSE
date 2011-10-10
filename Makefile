TARGET		:= busexmp
LIBOBJS 	:= buse.o
OBJS		:= $(TARGET:=.o) $(LIBOBJS)
SHAREDLIB	:= libbuse.so

CC			:= /usr/bin/gcc
CFLAGS		:= -g -pedantic -Wall -Wextra -std=c99 -I$(HOME)/local/include -I$(HOME)/src/nbd
LDFLAGS		:= -lbuse -L.

.PHONY: all clean
all: $(TARGET)

$(TARGET): %: %.o lib
	$(CC) $(LDFLAGS) -o $@ $<

$(TARGET:=.o): %.o: %.c buse.h
	$(CC) $(CFLAGS) -o $@ -c $<

lib: $(LIBOBJS)
	$(CC) -shared -fPIC -o $(SHAREDLIB) $^

$(LIBOBJS): %.o: %.c
	$(CC) $(CFLAGS) -fPIC -o $@ -c $<

clean:
	rm -f $(TARGET) $(OBJS) $(SHAREDLIB)
