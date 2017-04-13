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

struct led {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct ws2801_driver {
	void (*clear)(struct ws2801_driver *ws);

	void (*sync)(struct ws2801_driver *ws);

	int (*set_led)(struct ws2801_driver *ws, unsigned int num,
		       const struct led *led);
	int (*set_leds)(struct ws2801_driver *ws, const struct led *leds,
			unsigned int offset, unsigned int num_leds);

	int (*free)(struct ws2801_driver *ws);

	unsigned int num_leds;

	void *drv_data;
};

int ws2801_user_init(unsigned int num_leds, unsigned int gpiochip,
		     int gpio_clk, int gpio_do, struct ws2801_driver *ws);
