#ifndef ABUSE_H_INCLUDED
#define ABUSE_H_INCLUDED

#include <sys/types.h>

struct abuse_operations {
    void (*read)();
    void (*write)();
    void (*disc)();
    void (*flush)();
    void (*trim)();

    u_int64_t size;
};

int abuse_main(int argc, char *argv[], struct abuse_operations *aop);

#endif /* ABUSE_H_INCLUDED */
