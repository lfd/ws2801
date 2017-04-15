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

struct ws2801_user {
	struct led *leds;
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

	data.values[IDX_CLK] = 0;
	ret = ioctl(req_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
	if (ret == -1)
		return ret;

	return 0;
}

static inline void __ws2801_user_free(struct ws2801_user *ws)
{
	if (ws->leds)
		free(ws->leds);

	if (ws->fd != -1)
		close(ws->fd);

	free(ws);
}

static void ws2801_user_free(struct ws2801_driver *ws_driver)
{
	struct ws2801_user *ws = ws_driver->drv_data;

	__ws2801_user_free(ws);
}

static int ws2801_user_set_led(struct ws2801_driver *ws_driver,
			       unsigned int num, const struct led *led)
{
	struct ws2801_user *ws = ws_driver->drv_data;

	if (num >= ws_driver->num_leds)
		return -EINVAL;

	ws->leds[num] = *led;

	return 0;
}

static void ws2801_user_sync(struct ws2801_driver *ws_driver)
{
	struct ws2801_user *ws = ws_driver->drv_data;
	unsigned int i;
	int err;

#define SEND_LED(__color) \
	err = ws2801_byte(ws->req_fd, ws->leds[i].__color); \
	if (err < 0) { \
		fprintf(stderr, "ws2801: error during sync\n"); \
		exit(err); \
	}

	for (i = 0; i < ws_driver->num_leds; i++) {
		SEND_LED(r);
		SEND_LED(g);
		SEND_LED(b);
	}
#undef SEND_LED
}

static void ws2801_user_clear(struct ws2801_driver *ws_driver)
{
	struct ws2801_user *ws = ws_driver->drv_data;

	memset(ws->leds, 0, ws_driver->num_leds * sizeof(*ws->leds));
}

static int ws2801_user_set_leds(struct ws2801_driver *ws_driver,
				const struct led *leds, unsigned int offset,
				unsigned int num_leds)
{
	struct ws2801_user *ws = ws_driver->drv_data;

	if (offset >= ws_driver->num_leds)
		return 0;

	if (num_leds + offset >= ws_driver->num_leds)
		num_leds = ws_driver->num_leds - offset;

	memcpy(ws->leds + offset, leds, num_leds * sizeof(*leds));

	return num_leds;
}

int ws2801_user_init(unsigned int num_leds, unsigned int gpiochip, int gpio_clk,
		     int gpio_do, struct ws2801_driver *ws_driver)
{
	struct ws2801_user *ws;
	char *chrdev_name;
	int i, ret;
	struct gpiohandle_request req;

	if (!ws_driver || (gpio_clk == gpio_do))
		return -EINVAL;

	ret = asprintf(&chrdev_name, "/dev/gpiochip%u", gpiochip);
	if (ret < 0)
		return -ENOMEM;

	ws = calloc(1, sizeof(*ws));
	if (!ws) {
		ret = -ENOMEM;
		goto chrdev_name_out;
	}
	ws->fd = -1;

	ws->leds = calloc(num_leds, sizeof(struct led));
	if (!ws->leds) {
		ret = -ENOMEM;
		goto free_out;
	}

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

	ws_driver->free = ws2801_user_free;
	ws_driver->set_led = ws2801_user_set_led;
	ws_driver->set_leds = ws2801_user_set_leds;
	ws_driver->sync = ws2801_user_sync;
	ws_driver->clear = ws2801_user_clear;

	ws_driver->num_leds = num_leds;
	ws_driver->drv_data = ws;

	return 0;

free_out:
	__ws2801_user_free(ws);

chrdev_name_out:
	free(chrdev_name);

	return ret;
}
