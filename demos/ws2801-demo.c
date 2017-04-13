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

#define NUM_PIXELS 100
#define DEVICE "gpiochip0"
#define GPIO_CLK 160
#define GPIO_DO 162

int main(void)
{
	int err, i;
	struct ws2801_driver ws;
	const struct led led = {
		.r = 255,
		.g = 255,
		.b = 255,
	};

	err = ws2801_user_init(NUM_PIXELS, DEVICE, GPIO_CLK, GPIO_DO, &ws);
	if (err) {
		fprintf(stderr, "initialising ws2801: %s\n", strerror(-err));
		return err;
	}

	for (i = 0; ; i++) {
		ws.clear(&ws);

		err = ws.set_led(&ws, i % ws.num_leds, &led);
		if (err) {
			fprintf(stderr, "set led\n");
			break;
		}

		ws.sync(&ws);
		usleep(10000);
	}

	ws.free(&ws);
	return err;
}
