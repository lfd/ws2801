/*
 * ws2801 - WS2801 LED driver running in Linux userspace
 *
 * Copyright (c) - Ralf Ramsauer, 2017
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "ws2801-common.h"

static inline void ws2801_auto_commit(struct ws2801_driver *ws_driver)
{
	if (ws_driver->auto_commit)
		ws_driver->commit(ws_driver);
}

int ws2801_init(struct ws2801_driver *ws_driver, unsigned int num_leds)
{
	int err;

	ws_driver->leds = calloc(num_leds, sizeof(struct led));
	if (!ws_driver->leds)
		return -ENOMEM;

	ws_driver->num_leds = num_leds;

	err = pthread_mutex_init(&ws_driver->data_lock, NULL);
	if (err) {
		free(ws_driver->leds);
		return err;
	}

	ws2801_set_auto_commit(ws_driver, WS2801_DEFAULT_AUTO_COMMIT);

	return 0;
}

void ws2801_free(struct ws2801_driver *ws_driver)
{
	if (ws_driver->leds)
		free(ws_driver->leds);

	pthread_mutex_destroy(&ws_driver->data_lock);
}

void ws2801_set_auto_commit(struct ws2801_driver *ws_driver, bool auto_commit)
{
	ws_driver->auto_commit = auto_commit;
}

void ws2801_clear(struct ws2801_driver *ws_driver)
{
	pthread_mutex_lock(&ws_driver->data_lock);
	memset(ws_driver->leds, 0,
	       ws_driver->num_leds * sizeof(*ws_driver->leds));
	pthread_mutex_unlock(&ws_driver->data_lock);

	ws2801_auto_commit(ws_driver);
}

int ws2801_set_led(struct ws2801_driver *ws_driver, unsigned int num,
		   const struct led *led)
{
	pthread_mutex_lock(&ws_driver->data_lock);
	if (num >= ws_driver->num_leds) {
		pthread_mutex_unlock(&ws_driver->data_lock);
		return -ERANGE;
	}

	ws_driver->leds[num] = *led;
	pthread_mutex_unlock(&ws_driver->data_lock);

	ws2801_auto_commit(ws_driver);

	return 0;
}

int ws2801_set_leds(struct ws2801_driver *ws_driver, const struct led *leds,
		    unsigned int offset, unsigned int num_leds)
{
	if (offset >= ws_driver->num_leds)
		return 0;

	pthread_mutex_lock(&ws_driver->data_lock);

	if (num_leds + offset >= ws_driver->num_leds)
		num_leds = ws_driver->num_leds - offset;

	memcpy(ws_driver->leds + offset, leds, num_leds * sizeof(*leds));

	pthread_mutex_unlock(&ws_driver->data_lock);

	ws2801_auto_commit(ws_driver);

	return num_leds;
}

int ws2801_full_on(struct ws2801_driver *ws_driver, const struct led *color)
{
	unsigned int i;
	int err;

	for (i = 0; i < ws_driver->num_leds; i++) {
		err = ws_driver->set_led(ws_driver, i, color);
		if (err)
			break;
	}

	return err;
}
