#include <stdlib.h>
#include <string.h>

#include "abuse.h"

static int xmp_read(void *buf, u_int32_t len, u_int64_t offset)
{
    (void) offset;

    memset(buf, 0, len);

    return 0;
}

static struct abuse_operations aop = {
    .read = xmp_read,
    .size = 1024,
};

int main(int argc, char *argv[])
{
    return abuse_main(argc, argv, &aop, NULL);
}
