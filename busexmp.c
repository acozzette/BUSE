/*
 * busexmp - example memory-based block device using BUSE
 * Copyright (C) 2013 Adam Cozzette
 *
 * This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buse.h"

static void *data;
static int xmpl_debug = 1;

static int xmp_read(void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
  if (*(int *)userdata)
    fprintf(stderr, "R - %lu, %u\n", offset, len);
  memcpy(buf, (char *)data + offset, len);
  return 0;
}

static int xmp_write(const void *buf, u_int32_t len, u_int64_t offset, void *userdata)
{
  if (*(int *)userdata)
    fprintf(stderr, "W - %lu, %u\n", offset, len);
  memcpy((char *)data + offset, buf, len);
  return 0;
}

static void xmp_disc(void *userdata)
{
  (void)(userdata);
  fprintf(stderr, "Received a disconnect request.\n");
}

static int xmp_flush(void *userdata)
{
  (void)(userdata);
  fprintf(stderr, "Received a flush request.\n");
  return 0;
}

static int xmp_trim(u_int64_t from, u_int32_t len, void *userdata)
{
  (void)(userdata);
  fprintf(stderr, "T - %lu, %u\n", from, len);
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
  if (argc != 2)
  {
    fprintf(stderr, 
        "Usage:\n"
        "  %s /dev/nbd0\n"
        "Don't forget to load nbd kernel module (`modprobe nbd`) and\n"
        "run example from root.\n", argv[0]);
    return 1;
  }

  data = malloc(aop.size);

  return buse_main(argv[1], &aop, (void *)&xmpl_debug);
}
