#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buse.h"

static void *data;

static int xmp_read(void *buf, u_int32_t len, u_int64_t offset)
{
  fprintf(stderr, "R - %lu, %u\n", offset, len);
  memcpy(buf, (char *)data + offset, len);
  return 0;
}

static int xmp_write(const void *buf, u_int32_t len, u_int64_t offset)
{
  fprintf(stderr, "W - %lu, %u\n", offset, len);
  memcpy((char *)data + offset, buf, len);
  return 0;
}

static int xmp_disc()
{
  fprintf(stderr, "Received a disconnect request.\n");
  return 0;
}

static int xmp_flush(){
  fprintf(stderr, "F\n");
  return 0;
}

static int xmp_trim(u_int64_t from, u_int32_t len){
  fprintf(stderr, "T - %lu, %u\n", from, len);
  return 0;
}


static struct buse_operations aop = {
  .read = xmp_read,
  .write = xmp_write,
  .disc = xmp_disc,
  .trim = xmp_trim,
  .flush = xmp_flush,
  .size = 128 * 1024 * 1024,
};

int main(int argc, char *argv[])
{
  data = malloc(aop.size);

  return buse_main(argc, argv, &aop, NULL);
}
