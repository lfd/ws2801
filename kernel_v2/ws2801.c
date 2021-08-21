/*
 * ws2801 - WS2801 LED driver running in Linux kernelspace
 *
 * Copyright (c) - Ralf Ramsauer, 2021
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <linux/module.h>
#include <linux/platform_device.h>

#define DRIVER_NAME	"ws2801"

static struct device *ws2801_dev;

static int __init ws2801_module_init(void)
{
	ws2801_dev = root_device_register(DRIVER_NAME);
	if (IS_ERR(ws2801_dev))
		return PTR_ERR(ws2801_dev);

	return 0;
}

static void __exit ws2801_module_exit(void)
{
	root_device_unregister(ws2801_dev);
}

module_init(ws2801_module_init);
module_exit(ws2801_module_exit);

MODULE_AUTHOR("Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>");
MODULE_LICENSE("GPL");
