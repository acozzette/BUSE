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

static int xmp_disc()
{
    fprintf(stderr, "Received a disconnect request.\n");
    return 0;
}

static struct buse_operations aop = {
    .read = xmp_read,
    .write = xmp_write,
    .disc = xmp_disc,
    .size = 128 * 1024 * 1024,
};

int main(int argc, char *argv[])
{
    data = malloc(aop.size);

    return buse_main(argc, argv, &aop, NULL);
}
