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

#include <stdbool.h>

#define WS2801_DEFAULT_REFRESH_RATE 5000
#define WS2801_DEFAULT_AUTO_COMMIT false

struct led {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct ws2801_driver {
	/* The refresh rate (in ms) forces the driver to commit changes to the
	 * LED strip after a certain timeout, if no other changes were made.
	 *
	 * Returns 0 on success, and negative values in error cases.
	 */
	int (*set_refresh_rate)(struct ws2801_driver *ws, unsigned int refresh_rate_ms);

	/* Automatically commit any change immediately */
	void (*set_auto_commit)(struct ws2801_driver *ws, bool auto_commit);

	/* Set all LEDs to RGB (0, 0, 0) */
	void (*clear)(struct ws2801_driver *ws);

	/* Commit all pending changes to the hardware */
	void (*commit)(struct ws2801_driver *ws);

	/* Sets one single LED
	 *
	 * Returns 0 on success, and negative values in error cases.
	 */
	int (*set_led)(struct ws2801_driver *ws, unsigned int num,
		       const struct led *led);

	/* Set a range of LEDs
	 *
	 * Returns 0 on success, and negative values in error cases.
	 */
	int (*set_leds)(struct ws2801_driver *ws, const struct led *leds,
			unsigned int offset, unsigned int num_leds);

	/* Free the driver structure */
	void (*free)(struct ws2801_driver *ws);

	/* Holds the number of LEDs. Do not write access! */
	unsigned int num_leds;
	struct led *leds;

	/* Private driver data structure. Do not access! */
	void *drv_data;
};

int ws2801_user_init(unsigned int num_leds, unsigned int gpiochip,
		     int gpio_clk, int gpio_do, struct ws2801_driver *ws);
