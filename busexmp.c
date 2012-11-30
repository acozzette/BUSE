#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buse.h"

static void *data;

static int xmp_read(void *buf, u_int32_t len, u_int64_t offset)
{
  memcpy(buf, (char *)data + offset, len);
  fprintf(stderr, "R %lu %u\n", offset, len);
  return 0;
}

static int xmp_write(const void *buf, u_int32_t len, u_int64_t offset)
{
  memcpy((char *)data + offset, buf, len);
  fprintf(stderr, "W %lu %u\n", offset, len);
  return 0;
}

static int xmp_disc()
{
  fprintf(stderr, "Received a disconnect request.\n");
  return 0;
}

static int xmp_flush()
{
  fprintf(stderr, "Received a flush request.\n");
  return 0;
}

static int xmp_trim(u_int64_t from, u_int32_t len)
{
  fprintf(stderr, "T %lu %u\n", from, len);
  return 0;
}

static struct buse_operations aop = {
  .read = xmp_read,
  .write = xmp_write,
  .disc = xmp_disc,
  .flush = xmp_flush,
  .trim = xmp_trim,
  .size = 128 * 1024 * 1024,
};

int main(int argc, char *argv[])
{
  data = malloc(aop.size);

  return buse_main(argc, argv, &aop, NULL);
}
