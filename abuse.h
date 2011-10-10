#ifndef ABUSE_H_INCLUDED
#define ABUSE_H_INCLUDED

#include <sys/types.h>

struct abuse_operations {
    int (*read)(void *buf, u_int32_t len, u_int64_t offset);
    int (*write)(const void *buf, u_int32_t len, u_int64_t offset);
    int (*disc)();
    int (*flush)();
    int (*trim)();

    u_int64_t size;
};

int abuse_main(int argc, char *argv[], const struct abuse_operations *aop,
        void *userdata);

#endif /* ABUSE_H_INCLUDED */
