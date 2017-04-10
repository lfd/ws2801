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

#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/gpio.h>

#include "ws2801.h"

#define IDX_CLK 0
#define IDX_DO 1

#define INIT_CLEAR_MAX 100

struct ws2801 {
	size_t num_pixels;
	struct pixel *pixels;
	int fd;
	int req_fd;
};

static inline int ws2801_byte(int req_fd, unsigned char byte)
{
	int ret;
	unsigned char mask;
	struct gpiohandle_data data;

	for (mask = 0x80; mask; mask >>= 1) {
		data.values[IDX_CLK] = 0;
		data.values[IDX_DO] = (byte & mask) ? 1 : 0;

		ret = ioctl(req_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
		if (ret == -1)
			return ret;

		data.values[IDX_CLK] = 1;
		ret = ioctl(req_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
		if (ret == -1)
			return ret;
	}

	return 0;
}

int ws2801_init(size_t num_pixels, const char *device_name, int gpio_clk,
		int gpio_do, struct ws2801 **ws2801)
{
	struct ws2801 *ws;
	char *chrdev_name;
	int i, ret;
	struct gpiohandle_request req;

	if (!ws2801 || (gpio_clk == gpio_do))
		return -EINVAL;

	ret = asprintf(&chrdev_name, "/dev/%s", device_name);
	if (ret < 0)
		return -ENOMEM;

	ws = calloc(1, sizeof(struct ws2801));
	if (!ws) {
		ret = -ENOMEM;
		goto chrdev_name_out;
	}
	ws->fd = -1;

	ws->pixels = calloc(num_pixels, sizeof(struct pixel));
	if (!ws->pixels) {
		ret = -ENOMEM;
		goto free_out;
	}
	ws->num_pixels = num_pixels;

	ws->fd = open(chrdev_name, 0);
	if (ws->fd == -1) {
		ret = -errno;
		goto free_out;
	}

	strcpy(req.consumer_label, "ws2801");
	req.lineoffsets[IDX_CLK] = gpio_clk;
	req.lineoffsets[IDX_DO] = gpio_do;
	req.lines = 2;
	req.flags = GPIOHANDLE_REQUEST_OUTPUT;

	ret = ioctl(ws->fd, GPIO_GET_LINEHANDLE_IOCTL, &req);
	if (ret == -1) {
		ret = -errno;
		goto free_out;
	}

	ws->req_fd = req.fd;

	for (i = 0; i < INIT_CLEAR_MAX; i++) {
		ret = ws2801_byte(ws->req_fd, 0);
		if (ret)
			goto free_out;
	}

	*ws2801 = ws;
	return 0;

free_out:
	ws2801_free(ws);

chrdev_name_out:
	free(chrdev_name);

	return ret;
}

int ws2801_free(struct ws2801 *ws)
{
	if (ws->pixels)
		free(ws->pixels);

	if (ws->fd != -1)
		close(ws->fd);

	free(ws);
}

int ws2801_set_pixel(struct ws2801 *ws, size_t num, struct pixel *pixel)
{
	if (num >= ws->num_pixels)
		return -EINVAL;

	ws->pixels[num] = *pixel;

	return 0;
}

int ws2801_update(struct ws2801 *ws)
{
	int i, ret;
	struct gpiohandle_data data;


#define SEND_PIXEL(__color) \
	ret = ws2801_byte(ws->req_fd, ws->pixels[i].__color); \
	if (ret < 0) \
		return ret;

	for (i = 0; i < ws->num_pixels; i++) {
		SEND_PIXEL(r);
		SEND_PIXEL(g);
		SEND_PIXEL(b);
	}

	// CHECK: set data to zero?

#undef SEND_PIXEL

	return 0;
}

int ws2801_set_pixels(struct ws2801 *ws, struct pixel *pixel, off_t offset,
		      size_t num_pixels)
{
	if (offset >= ws->num_pixels)
		return 0;

	if (num_pixels + offset >= ws->num_pixels)
		num_pixels = ws->num_pixels - offset;

	memcpy(ws->pixels + offset, pixel, num_pixels * sizeof(struct pixel));

	return num_pixels;
}
