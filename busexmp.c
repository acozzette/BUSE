#include <stdlib.h>
#include <string.h>

#include "buse.h"

static void *data;

static int xmp_read(void *buf, u_int32_t len, u_int64_t offset)
{
    memcpy(buf, (char *)data + offset, len);
    return 0;
}

static int xmp_write(const void *buf, u_int32_t len, u_int64_t offset)
{
    memcpy((char *)data + offset, buf, len);
    return 0;
}

static struct buse_operations aop = {
    .read = xmp_read,
    .write = xmp_write,
    .size = 128 * 1024,
};

int main(int argc, char *argv[])
{
    data = malloc(aop.size);

    return buse_main(argc, argv, &aop, NULL);
}
