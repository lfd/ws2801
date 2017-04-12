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

struct pixel {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct ws2801_driver {
	int (*update)(struct ws2801_driver *ws);

	int (*set_pixel)(struct ws2801_driver *ws, size_t num,
			 struct pixel *pixel);
	int (*set_pixels)(struct ws2801_driver *ws, struct pixel *pixel,
			  off_t offset, size_t num_pixels);

	int (*free)(struct ws2801_driver *ws);

	void *drv_data;
};

int ws2801_user_init(size_t num_pixels, const char *device_name, int gpio_clk,
		     int gpio_do, struct ws2801_driver *ws);
