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

struct ws2801;

struct pixel {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

int ws2801_user_init(size_t num_pixels, const char *device_name, int gpio_clk,
		     int gpio_do, struct ws2801 **ws2801);
int ws2801_free(struct ws2801 *ws);
int ws2801_set_pixel(struct ws2801 *ws, size_t num, struct pixel *pixel);
int ws2801_update(struct ws2801 *ws);
int ws2801_set_pixels(struct ws2801 *ws, struct pixel *pixel, off_t offset,
		      size_t num_pixels);
