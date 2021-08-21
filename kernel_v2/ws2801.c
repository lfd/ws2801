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
#include <linux/of.h>
#include <linux/platform_device.h>

#define DRIVER_NAME	"ws2801"

struct ws2801 {
	struct gpio_desc *clk;
	struct gpio_desc *data;
};

static struct device *ws2801_dev;

static int ws2801_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct ws2801 *ws;

	ws = devm_kzalloc(dev, sizeof(*ws), GFP_KERNEL);
	if (!ws)
		return -ENOMEM;
	platform_set_drvdata(pdev, ws);

	return 0;
}

static int ws2801_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id of_ws2801_match[] = {
	{
		.compatible = DRIVER_NAME,
	},
	{},
};

MODULE_DEVICE_TABLE(of, of_ws2801_match);

static struct platform_driver ws2801_driver = {
	.probe = ws2801_probe,
	.remove = ws2801_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = of_ws2801_match,
	},
};

static int __init ws2801_module_init(void)
{
	int err;

	ws2801_dev = root_device_register(DRIVER_NAME);
	if (IS_ERR(ws2801_dev))
		return PTR_ERR(ws2801_dev);

	err = platform_driver_register(&ws2801_driver);
	if (err)
		goto dev_out;

	return 0;

dev_out:
	root_device_unregister(ws2801_dev);
	return err;
}

static void __exit ws2801_module_exit(void)
{
	platform_driver_unregister(&ws2801_driver);
	root_device_unregister(ws2801_dev);
}

module_init(ws2801_module_init);
module_exit(ws2801_module_exit);

MODULE_AUTHOR("Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>");
MODULE_LICENSE("GPL");
