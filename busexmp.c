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

#include <argp.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buse.h"

/* BUSE callbacks */
static void *data;

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
  if (*(int *)userdata)
    fprintf(stderr, "Received a disconnect request.\n");
}

static int xmp_flush(void *userdata)
{
  if (*(int *)userdata)
    fprintf(stderr, "Received a flush request.\n");
  return 0;
}

static int xmp_trim(u_int64_t from, u_int32_t len, void *userdata)
{
  if (*(int *)userdata)
    fprintf(stderr, "T - %lu, %u\n", from, len);
  return 0;
}

/* argument parsing using argp */

static struct argp_option options[] = {
  {"verbose", 'v', 0, 0, "Produce verbose output", 0},
  {0},
};

struct arguments {
  unsigned long long size;
  char * device;
  int verbose;
};

static unsigned long long strtoull_with_prefix(const char * str, char * * end) {
  unsigned long long v = strtoull(str, end, 0);
  switch (**end) {
    case 'K':
      v *= 1024;
      *end += 1;
      break;
    case 'M':
      v *= 1024 * 1024;
      *end += 1;
      break;
    case 'G':
      v *= 1024 * 1024 * 1024;
      *end += 1;
      break;
  }
  return v;
}

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state) {
  struct arguments *arguments = state->input;
  char * endptr;

  switch (key) {

    case 'v':
      arguments->verbose = 1;
      break;

    case ARGP_KEY_ARG:
      switch (state->arg_num) {

        case 0:
          arguments->size = strtoull_with_prefix(arg, &endptr);
          if (*endptr != '\0') {
            /* failed to parse integer */
            errx(EXIT_FAILURE, "SIZE must be an integer");
          }
          break;

        case 1:
          arguments->device = arg;
          break;

        default:
          /* Too many arguments. */
          return ARGP_ERR_UNKNOWN;
      }
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 2) {
        warnx("not enough arguments");
        argp_usage(state);
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp argp = {
  .options = options,
  .parser = parse_opt,
  .args_doc = "SIZE DEVICE",
  .doc = "BUSE virtual block device that stores its content in memory.\n"
         "`SIZE` accepts suffixes K, M, G. `DEVICE` is path to block device, for example \"/dev/nbd0\".",
};


int main(int argc, char *argv[]) {
  struct arguments arguments = {
    .verbose = 0,
  };
  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  struct buse_operations aop = {
    .read = xmp_read,
    .write = xmp_write,
    .disc = xmp_disc,
    .flush = xmp_flush,
    .trim = xmp_trim,
    .size = arguments.size,
  };

  data = malloc(aop.size);
  if (data == NULL) err(EXIT_FAILURE, "failed to alloc space for data");

  return buse_main(arguments.device, &aop, (void *)&arguments.verbose);
}
