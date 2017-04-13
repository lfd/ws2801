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

#include <stdio.h>
#include <string.h>
#include <ws2801.h>

#include "common.h"

#define NUM_PIXELS 10
#define DEVICE "gpiochip0"
#define GPIO_CLK 57
#define GPIO_DO 58

int main(void)
{
	int err;
	struct ws2801_driver ws;

	err = ws2801_user_init(NUM_PIXELS, DEVICE, GPIO_CLK, GPIO_DO, &ws);
	if (err) {
		fprintf(stderr, "initialising ws2801: %s\n", strerror(-err));
		return err;
	}

	err = app(&ws);

	ws.free(&ws);

	return err;
}
