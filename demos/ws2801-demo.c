/*
 * ws2801 - WS2801 LED driver running in Linux userspace
 *
 * Copyright (c) - Marvin Vierling, 2017
 * Copyright (c) - Ralf Ramsauer, 2017
 *
 * Authors:
 *   Marvin Vierling <marvin.vierling@st.oth-regensburg.de>
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <ws2801.h>

#include "common.h"

int app(struct ws2801_driver *ws)
{
	int err, i;
	const struct led led = {
		.r = 255,
		.g = 255,
		.b = 255,
	};

	for (i = 0; ; i++) {
		ws->clear(ws);

		err = ws->set_led(ws, i % ws->num_leds, &led);
		if (err) {
			fprintf(stderr, "set led\n");
			break;
		}

		ws->sync(ws);
		usleep(10000);
	}

	return err;
}
