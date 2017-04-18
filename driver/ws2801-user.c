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
#include <sys/time.h>
#include <linux/gpio.h>

#include "ws2801.h"

#define IDX_CLK 0
#define IDX_DO 1

#define INIT_CLEAR_MAX 100

struct ws2801_user {
	pthread_cond_t cond;
	pthread_mutex_t commit_lock;
	pthread_t refresh_task;

	volatile unsigned int refresh_rate;

	int fd;
	int req_fd;
};

static int msleep(pthread_cond_t *cond, pthread_mutex_t *mutex,
		  unsigned int ms)
{
	struct timeval now;
	struct timespec then;
	int err;

	then.tv_sec = ms / 1000UL;
	then.tv_nsec = (ms % 1000UL) * 1000000UL;

	err = gettimeofday(&now, NULL);
	if (err) {
		fprintf(stderr, "Error getting time of day\n");
		exit(-err);
	}

	then.tv_sec += now.tv_sec;
	then.tv_nsec += now.tv_usec * 1000UL;

	then.tv_sec += then.tv_nsec / 1000000000UL;
	then.tv_nsec %= 1000000000UL;

	return pthread_cond_timedwait(cond, mutex, &then);
}

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

static inline int ws2801_latch(int req_fd)
{
	struct gpiohandle_data data;

	data.values[IDX_CLK] = 0;
	return ioctl(req_fd, GPIOHANDLE_SET_LINE_VALUES_IOCTL, &data);
}

static void ws2801_user_commit(struct ws2801_driver *ws_driver)
{
	struct ws2801_user *ws = ws_driver->drv_data;
	unsigned int i;
	int err;

	pthread_mutex_lock(&ws->commit_lock);

#define SEND_LED(__color) \
	err = ws2801_byte(ws->req_fd, ws_driver->leds[i].__color); \
	if (err < 0) { \
		fprintf(stderr, "ws2801: error during commit\n"); \
		exit(err); \
	}

	for (i = 0; i < ws_driver->num_leds; i++) {
		SEND_LED(r);
		SEND_LED(g);
		SEND_LED(b);
	}
#undef SEND_LED

	err = ws2801_latch(ws->req_fd);
	if (err) {
		fprintf(stderr, "ws2801: error during commit\n");
		exit(err);
	}

	usleep(1000);

	pthread_mutex_unlock(&ws->commit_lock);
}

static int ws2801_user_set_led(struct ws2801_driver *ws_driver,
			       unsigned int num, const struct led *led)
{
	int err = 0;

	pthread_mutex_lock(&ws_driver->data_lock);
	if (num >= ws_driver->num_leds) {
		err = -ERANGE;
		goto unlock_out;
	}

	ws_driver->leds[num] = *led;

	if (ws_driver->auto_commit)
		ws2801_user_commit(ws_driver);

unlock_out:
	pthread_mutex_unlock(&ws_driver->data_lock);
	return err;
}

static void *ws2801_refresh_task(void *data)
{
	struct ws2801_driver *ws_driver = data;
	struct ws2801_user *ws = ws_driver->drv_data;
	int err;

	while (1) {
		pthread_mutex_lock(&ws_driver->data_lock);

		err = msleep(&ws->cond, &ws_driver->data_lock, ws->refresh_rate);
		if (err && err != ETIMEDOUT) {
			goto unlock_out;
		}

		if (!ws->refresh_rate) {
			err = 0;
			goto unlock_out;
		}

		pthread_mutex_unlock(&ws_driver->data_lock);
		ws2801_user_commit(ws_driver);
	}

unlock_out:
	pthread_mutex_unlock(&ws_driver->data_lock);
	return (void*)(long)err;
}

static int ws2801_user_set_refresh_rate(struct ws2801_driver *ws_driver,
					unsigned int refresh_rate)
{
	struct ws2801_user *ws = ws_driver->drv_data;
	unsigned int old = ws->refresh_rate;
	int err;

	if (old == refresh_rate)
		return 0;

	ws->refresh_rate = refresh_rate;

	if (!old && refresh_rate) {
		/* start auto update task */
		err = pthread_create(&ws->refresh_task, NULL,
				     ws2801_refresh_task, ws_driver);
		if (err)
			return err;
	} else if (old && !refresh_rate) {
		/* stop thread */
		err = pthread_cond_signal(&ws->cond);
		if (err)
			return err;
		err = pthread_join(ws->refresh_task, NULL);
		if (err)
			return err;
	} else {
		/* notify thread */
		err = pthread_cond_signal(&ws->cond);
		if (err)
			return err;
	}

	return 0;
}

static void ws2801_user_clear(struct ws2801_driver *ws_driver)
{
	pthread_mutex_lock(&ws_driver->data_lock);
	memset(ws_driver->leds, 0,
	       ws_driver->num_leds * sizeof(*ws_driver->leds));
	pthread_mutex_unlock(&ws_driver->data_lock);

	if (ws_driver->auto_commit)
		ws2801_user_commit(ws_driver);
}

static int ws2801_user_set_leds(struct ws2801_driver *ws_driver,
				const struct led *leds, unsigned int offset,
				unsigned int num_leds)
{
	if (offset >= ws_driver->num_leds)
		return 0;

	pthread_mutex_lock(&ws_driver->data_lock);

	if (num_leds + offset >= ws_driver->num_leds)
		num_leds = ws_driver->num_leds - offset;

	memcpy(ws_driver->leds + offset, leds, num_leds * sizeof(*leds));

	pthread_mutex_unlock(&ws_driver->data_lock);

	if (ws_driver->auto_commit)
		ws2801_user_commit(ws_driver);

	return num_leds;
}

static void ws2801_user_set_auto_commit(struct ws2801_driver *ws_driver,
					bool auto_commit)
{
	ws_driver->auto_commit = auto_commit;
}

static void ws2801_user_free(struct ws2801_driver *ws_driver)
{
	struct ws2801_user *ws = ws_driver->drv_data;
	int err;

	err = ws2801_user_set_refresh_rate(ws_driver, 0);
	if (err) {
		fprintf(stderr, "fatal: stopping auto update thread\n");
		exit(-err);
	}

	if (ws_driver->leds)
		free(ws_driver->leds);

	pthread_mutex_destroy(&ws_driver->data_lock);
	pthread_mutex_destroy(&ws->commit_lock);
	pthread_cond_destroy(&ws->cond);

	if (ws->fd != -1)
		close(ws->fd);

	free(ws);
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
	ws_driver->drv_data = ws;

	ret = pthread_mutex_init(&ws_driver->data_lock, NULL);
	if (ret)
		goto free_ws_out;

	ret = pthread_mutex_init(&ws->commit_lock, NULL);
	if (ret)
		goto free_data_lock_out;

	ret = pthread_cond_init(&ws->cond, NULL);
	if (ret)
		goto free_commit_lock_out;

	ws_driver->leds = calloc(num_leds, sizeof(struct led));
	if (!ws_driver->leds) {
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

	ws_driver->clear = ws2801_user_clear;
	ws_driver->set_auto_commit = ws2801_user_set_auto_commit;
	ws_driver->set_led = ws2801_user_set_led;
	ws_driver->set_leds = ws2801_user_set_leds;
	ws_driver->set_refresh_rate = ws2801_user_set_refresh_rate;
	ws_driver->commit = ws2801_user_commit;
	ws_driver->free = ws2801_user_free;

	ws_driver->num_leds = num_leds;

	ret = ws2801_user_set_refresh_rate(ws_driver,
					   WS2801_DEFAULT_REFRESH_RATE);
	if (ret) {
		ws2801_user_free(ws_driver);
		return ret;
	}

	return 0;

free_out:
	ws2801_user_free(ws_driver);
	free(chrdev_name);

	return ret;

free_commit_lock_out:
	pthread_mutex_destroy(&ws->commit_lock);

free_data_lock_out:
	pthread_mutex_destroy(&ws_driver->data_lock);

free_ws_out:
	free(ws);

chrdev_name_out:
	free(chrdev_name);

	return ret;
}
