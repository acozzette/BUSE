#ifndef BUSE_H_INCLUDED
#define BUSE_H_INCLUDED

#include <sys/types.h>

struct buse_operations {
    int (*read)(void *buf, u_int32_t len, u_int64_t offset);
    int (*write)(const void *buf, u_int32_t len, u_int64_t offset);
    int (*disc)();
    int (*flush)();
    int (*trim)(u_int64_t from, u_int32_t len);

    u_int64_t size;
};

int buse_main(int argc, char *argv[], const struct buse_operations *aop,
        void *userdata);

#endif /* BUSE_H_INCLUDED */
