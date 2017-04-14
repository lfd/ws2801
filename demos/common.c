/*
 * ws2801 - WS2801 LED driver running in Linux userspace
 *
 * Copyright (c) - Ralf Ramsauer, 2017
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ws2801.h>

#include "common.h"

#define GPIOCHIP "gpiochip"

#define DEFAULT_NUM_LEDS 20
#define DEFAULT_GPIOCHIP 0

static void __attribute__((noreturn)) usage(int exit_code)
{
	FILE *s;

	if (exit_code)
		s = stderr;
	else
		s = stdout;

	fprintf(s, "Usage: { -c CLK_GPIO_ID } { -d DATA_GPIO_ID }\n"
		   "       [ -n NUM_LEDS (20) ]\n"
		   "       [ -g CHIP_ID (0) ]\n"
		   "       [ -h ]\n");

	exit(exit_code);
}

int main(int argc, char **argv)
{
	int err;
	int option;
	struct ws2801_driver ws;
	int clock = -1, data = -1;
	unsigned int chip = DEFAULT_GPIOCHIP;
	unsigned int num_leds = DEFAULT_NUM_LEDS;

	option = 0;

	while ((option = getopt(argc, argv, "c:d:n:g:h")) != -1) {
		switch (option) {
			case 'c':
				clock = atoi(optarg);
				break;
			case 'd':
				data = atoi(optarg);
				break;
			case 'n':
				num_leds = atoi(optarg);
				break;
			case 'g':
				chip = atoi(optarg);
				break;
			default:
				usage(-1);
		}
	}

	if (clock == -1 || data == -1)
		usage(-EINVAL);

	err = ws2801_user_init(num_leds, chip, clock, data, &ws);
	if (err) {
		fprintf(stderr, "initialising ws2801: %s\n", strerror(-err));
		return err;
	}

	err = app(&ws);

	ws.free(&ws);

	return err;
}
