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

#include "ws2801.h"

int ws2801_init(struct ws2801_driver *ws_driver, unsigned int num_leds);

void ws2801_free(struct ws2801_driver *ws_driver);

void ws2801_set_auto_commit(struct ws2801_driver *ws_driver, bool auto_commit);

void ws2801_clear(struct ws2801_driver *ws_driver);

int ws2801_set_led(struct ws2801_driver *ws_driver, unsigned int num,
		   const struct led *led);

int ws2801_set_leds(struct ws2801_driver *ws_driver, const struct led *leds,
		    unsigned int offset, unsigned int num_leds);
