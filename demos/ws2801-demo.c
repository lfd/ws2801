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

static struct led leds[NUM_PIXELS];

int main(void)
{
	int err, i = 0;
	struct ws2801_driver ws;

	err = ws2801_user_init(NUM_PIXELS, DEVICE, GPIO_CLK, GPIO_DO, &ws);
	if (err) {
		fprintf(stderr, "initialising ws2801: %s\n", strerror(-err));
		return err;
	}

	for (;;) {
		memset(leds, 0, sizeof(leds));

		leds[i % NUM_PIXELS].r = 255;
		leds[i % NUM_PIXELS].g = 255;
		leds[i % NUM_PIXELS].b = 255;

		i++;

		err = ws.set_leds(&ws, leds, 0, NUM_PIXELS);
		err = ws.sync(&ws);
		if (err)
			break;
		usleep(10000);
	}

	ws.free(&ws);
	return 0;
}
