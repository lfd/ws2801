/*
 * ws2801 - WS2801 LED driver running in Linux kernelspace
 *
 * Copyright (c) - Ralf Ramsauer, 2017
 *
 * Authors:
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <linux/delay.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio/consumer.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/kthread.h>

#define DRIVER_NAME "ws2801"

#define WS2801_NUM_LEDS_DEFAULT 30
#define WS2801_DEFAULT_REFRESH_RATE 5000

#define INIT_CLEAR_MAX 1000

#define FORMAT_LED "%hhu %hhu %hhu"
#define FORMAT_NUM_LED "%u " FORMAT_LED

static struct device *ws2801_dev;
static struct kobject *devices_dir;

struct led {
	unsigned char r;
	unsigned char g;
	unsigned char b;
};

struct ws2801 {
	struct kobject kobj;
	struct device *dev;
	struct mutex data_lock;
	struct mutex commit_lock;
	struct task_struct *refresh_task;
	bool auto_commit;

	char name[16];
	unsigned int refresh_rate; /* in ms. 0: off */
	unsigned int num_leds;
	struct led *leds;
	struct gpio_desc *clk;
	struct gpio_desc *data;
};

const static struct led blank_led = {
	.r = 0,
	.g = 0,
	.b = 0,
};

static inline void ws2801_byte(struct ws2801 *ws, unsigned char byte)
{
	unsigned char mask;

	for (mask = 0x80; mask; mask >>= 1) {
		gpiod_set_value(ws->clk, 0);
		gpiod_set_value(ws->data, (byte & mask) ? 1 : 0);
		gpiod_set_value(ws->clk, 1);
		udelay(1);
	}
}

static inline void ws2801_send_led(struct ws2801 *ws, const struct led *led)
{
	ws2801_byte(ws, led->r);
	ws2801_byte(ws, led->g);
	ws2801_byte(ws, led->b);
}

static inline void ws2801_set_latch(struct ws2801 *ws)
{
	gpiod_set_value(ws->clk, 0);
	udelay(1000);
}

static int ws2801_set_led(struct ws2801 *ws, size_t no, struct led *led)
{
	if (no >= ws->num_leds)
		return -ERANGE;

	ws->leds[no] = *led;

	return 0;
}

static inline void ws2801_set_leds(struct ws2801 *ws, const struct led *led)
{
	unsigned int i;

	for (i = 0; i < ws->num_leds; i++)
		ws->leds[i] = *led;
}

static void ws2801_commit(struct ws2801 *ws, const struct led *leds,
			  unsigned int num_leds)
{
	unsigned int i;

	mutex_lock(&ws->commit_lock);
	for (i = 0; i < num_leds; i++)
		ws2801_send_led(ws, leds + i);

	ws2801_set_latch(ws);
	mutex_unlock(&ws->commit_lock);
}

static inline void ws2801_clear(struct ws2801 *ws)
{
	memset(ws->leds, 0, ws->num_leds * sizeof(*ws->leds));
}

static int ws2801_refresh_thread(void *data)
{
	struct ws2801 *ws = data;
	struct led *leds, *new_leds;
	unsigned int msecs, num_leds;
	int err = 0;

	/* get a local copy of the LEDs. This minimises locked sections */
	mutex_lock(&ws->data_lock);
	num_leds = ws->num_leds;
	leds = kmemdup(ws->leds, num_leds * sizeof(*leds), GFP_KERNEL);
	if (!leds) {
		mutex_unlock(&ws->data_lock);
		return -ENOMEM;
	}
	mutex_unlock(&ws->data_lock);

	while (1) {
		if (kthread_should_stop())
			break;

		if (mutex_lock_interruptible(&ws->data_lock) != 0)
			continue;

		msecs = ws->refresh_rate;

		/* check if number of LEDs changed */
		if (ws->num_leds != num_leds) {
			num_leds = ws->num_leds;
			new_leds = krealloc(leds, num_leds * sizeof(*leds),
					    GFP_KERNEL);
			if (!new_leds) {
				err = -ENOMEM;
				break;
			}
			leds = new_leds;
		}

		/* copy current LED configuration */
		memcpy(leds, ws->leds, num_leds * sizeof(*leds));

		mutex_unlock(&ws->data_lock);

		/* we can now safely update leds without holding the lock */
		ws2801_commit(ws, leds, num_leds);

		schedule_timeout_interruptible(msecs_to_jiffies(msecs));
	}

	kfree(leds);

	return err;
}

static int ws2801_set_refresh_rate(struct ws2801 *ws, unsigned int refresh_rate)
{
	unsigned int old;

	old = ws->refresh_rate;

	if (old == refresh_rate)
		return 0;

	ws->refresh_rate = refresh_rate;

	if (refresh_rate && !old) {
		/* start kthread */
		ws->refresh_task = kthread_run(ws2801_refresh_thread, ws,
					       DRIVER_NAME "-%s", ws->name);
		if (IS_ERR(ws->refresh_task))
			return PTR_ERR(ws->refresh_task);
	} else if (!refresh_rate && old) {
		/* stop kthread */
		return kthread_stop(ws->refresh_task);
	} else {
		/* notify task that update time has changed */
		wake_up_process(ws->refresh_task);
	}

	return 0;
}

static inline int ws2801_sysfs_parse_num_led(const char *str, unsigned int *num,
				             struct led *led)
{
	if (sscanf(str, FORMAT_NUM_LED, num, &led->r, &led->g, &led->b) != 4)
		return -EINVAL;

	return 0;
}

static inline int ws2801_sysfs_parse_led(const char *str, struct led *led)
{
	if (sscanf(str, FORMAT_LED, &led->r, &led->g, &led->b) != 3)
		return -EINVAL;

	return 0;
}

#define ATTR_SHOW_EINVAL(__attr) \
	static ssize_t __attr ## _show(struct kobject *kobj, \
				       struct kobj_attribute *attr, char *buf) \
	{ \
		return -EINVAL; \
	}


static ssize_t auto_commit_store(struct kobject *kobj, struct kobj_attribute *attr,
				 const char *buf, size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);
	bool auto_commit;
	int err;

	err = kstrtobool(buf, &auto_commit);
	if (err)
		return err;

	mutex_lock(&ws->data_lock);
	ws->auto_commit = auto_commit;
	mutex_unlock(&ws->data_lock);

	return len;
}

static ssize_t auto_commit_show(struct kobject *kobj, struct kobj_attribute *attr,
				char *buf)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);

	return sprintf(buf, "%u\n", ws->auto_commit);
}

ATTR_SHOW_EINVAL(clear);

static ssize_t clear_store(struct kobject *kobj, struct kobj_attribute *attr,
			  const char *buf, size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);

	mutex_lock(&ws->data_lock);
	ws2801_clear(ws);
	if (ws->auto_commit)
		ws2801_commit(ws, ws->leds, ws->num_leds);
	mutex_unlock(&ws->data_lock);

	return len;
}

ATTR_SHOW_EINVAL(full_on);

static ssize_t full_on_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);
	struct led led;
	int err;

	err = ws2801_sysfs_parse_led(buf, &led);
	if (err < 0)
		return err;

	mutex_lock(&ws->data_lock);
	ws2801_set_leds(ws, &led);
	if (ws->auto_commit)
		ws2801_commit(ws, ws->leds, ws->num_leds);
	mutex_unlock(&ws->data_lock);

	return len;
}

static ssize_t num_leds_show(struct kobject *kobj, struct kobj_attribute *attr,
			     char *buf)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);

	return sprintf(buf, "%u\n", ws->num_leds);
}

static ssize_t num_leds_store(struct kobject *kobj, struct kobj_attribute *attr,
			      const char *buf, size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);
	struct led *new;
	unsigned int num_leds;
	int err;

	err = kstrtouint(buf, 0, &num_leds);
	if (err)
		return err;

	mutex_lock(&ws->data_lock);

	new = devm_kzalloc(ws->dev, num_leds * sizeof(*ws->leds), GFP_KERNEL);
	if (!new) {
		err = -ENOMEM;
		goto unlock_out;
	}

	memcpy(new, ws->leds, min(num_leds, ws->num_leds) * sizeof(*new));

	devm_kfree(ws->dev, ws->leds);
	ws->num_leds = num_leds;
	ws->leds = new;

	err = len;

unlock_out:
	mutex_unlock(&ws->data_lock);

	return err;
}

static ssize_t refresh_rate_store(struct kobject *kobj,
				  struct kobj_attribute *attr, const char *buf,
				  size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);
	int err;
	unsigned int refresh_rate;

	err = kstrtouint(buf, 0, &refresh_rate);
	if (err)
		return -EINVAL;

	mutex_lock(&ws->data_lock);
	err = ws2801_set_refresh_rate(ws, refresh_rate);
	mutex_unlock(&ws->data_lock);

	if (!err)
		err = len;

	return err;
}

static ssize_t refresh_rate_show(struct kobject *kobj,
				 struct kobj_attribute *attr, char *buf)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);

	return sprintf(buf, "%u\n", ws->refresh_rate);
}

ATTR_SHOW_EINVAL(set);

static ssize_t set_store(struct kobject *kobj, struct kobj_attribute *attr,
			 const char *buf, size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);
	int err;

	unsigned int num;
	struct led led;

	mutex_lock(&ws->data_lock);

	do {
		err = ws2801_sysfs_parse_num_led(buf, &num, &led);
		if (err)
			break;
		err = ws2801_set_led(ws, num, &led);
		if (err)
			break;

		while (*buf && *buf != '\n')
			buf++;
		if (*buf) /* skip newline */
			buf++;
	} while (*buf);

	if (!err) {
		err = len;
		if (ws->auto_commit)
			ws2801_commit(ws, ws->leds, ws->num_leds);
	}

	mutex_unlock(&ws->data_lock);

	return err;
}

ATTR_SHOW_EINVAL(commit);

static ssize_t commit_store(struct kobject *kobj, struct kobj_attribute *attr,
			    const char *buf, size_t len)
{
	struct ws2801 *ws = container_of(kobj, struct ws2801, kobj);

	mutex_lock(&ws->data_lock);
	ws2801_commit(ws, ws->leds, ws->num_leds);
	mutex_unlock(&ws->data_lock);

	return len;
}

static struct kobj_attribute auto_commit_attr = __ATTR_RW(auto_commit);
static struct kobj_attribute clear_attr = __ATTR_RW(clear);
static struct kobj_attribute commit_attr = __ATTR_RW(commit);
static struct kobj_attribute full_on_attr = __ATTR_RW(full_on);
static struct kobj_attribute num_leds_attr = __ATTR_RW(num_leds);
static struct kobj_attribute refresh_rate_attr = __ATTR_RW(refresh_rate);
static struct kobj_attribute set_attr = __ATTR_RW(set);

static struct attribute *ws2801_per_device_attrs[] = {
	&auto_commit_attr.attr,
	&clear_attr.attr,
	&commit_attr.attr,
	&full_on_attr.attr,
	&num_leds_attr.attr,
	&refresh_rate_attr.attr,
	&set_attr.attr,
	NULL
};

static struct kobj_type device_type = {
	.sysfs_ops = &kobj_sysfs_ops,
	.default_attrs = ws2801_per_device_attrs,
};

static struct attribute *ws2801_sysfs_entries[] = {
	NULL
};

static struct attribute_group ws2801_attribute_group = {
	.name = NULL,
	.attrs = ws2801_sysfs_entries,
};

static void ws2801_sysfs_exit(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &ws2801_attribute_group);
}

static int ws2801_sysfs_init(struct device *dev)
{
	int err;

	err = sysfs_create_group(&dev->kobj, &ws2801_attribute_group);
	if (err)
		return err;

	devices_dir = kobject_create_and_add("devices", &dev->kobj);
	if (!devices_dir) {
		sysfs_remove_group(&dev->kobj, &ws2801_attribute_group);
		return -ENOMEM;
	}

	return 0;

}

static void ws2801_init_clear(struct ws2801 *ws)
{
	unsigned int i;

	mutex_lock(&ws->commit_lock);
	for (i = 0; i < INIT_CLEAR_MAX; i++)
		ws2801_send_led(ws, &blank_led);
	ws2801_set_latch(ws);
	mutex_unlock(&ws->commit_lock);
}

static int ws2801_remove(struct platform_device *pdev)
{
	struct ws2801 *ws = platform_get_drvdata(pdev);
	int err;

	mutex_lock(&ws->data_lock);
	err = ws2801_set_refresh_rate(ws, 0);
	ws2801_init_clear(ws);
	mutex_unlock(&ws->data_lock);

	return err;
}

static int ws2801_probe(struct platform_device *pdev)
{
	struct ws2801 *ws;
	struct device *dev = &pdev->dev;
	int i, err;
	unsigned int refresh_rate;

	ws = devm_kzalloc(dev, sizeof(*ws), GFP_KERNEL);
	if (!ws)
		return -ENOMEM;

	ws->dev = dev;

	mutex_init(&ws->data_lock);
	mutex_init(&ws->commit_lock);

	i = of_property_read_u32(dev->of_node, "num-leds", &ws->num_leds);
	if (i) {
		ws->num_leds = WS2801_NUM_LEDS_DEFAULT;
		dev_warn(dev, "error reading num_leds: %d, defaulting to %u\n",
			 i, ws->num_leds);
	}

	i = of_property_read_u32(dev->of_node, "refresh-rate", &refresh_rate);
	if (i) {
		refresh_rate = WS2801_DEFAULT_REFRESH_RATE;
		dev_warn(dev,
			 "error reading refresh-rate: %d, defaulting to %ums\n",
			 i, refresh_rate);
	}

	ws->leds = devm_kzalloc(dev, ws->num_leds * sizeof(struct led),
				GFP_KERNEL);
	if (!ws->leds)
		return -ENOMEM;

	ws->clk = devm_gpiod_get(dev, "clk", GPIOD_OUT_LOW);
	if (IS_ERR(ws->clk)) {
		dev_err(dev, "error getting clk: %ld", PTR_ERR(ws->clk));
		return PTR_ERR(ws->clk);
	}

	ws->data = devm_gpiod_get(dev, "data", GPIOD_OUT_LOW);
	if (IS_ERR(ws->data)) {
		dev_err(dev, "error getting data: %ld", PTR_ERR(ws->data));
		return PTR_ERR(ws->data);
	}

	ws->auto_commit = of_property_read_bool(dev->of_node, "auto-commit");


	strncpy(ws->name, dev->of_node->name, sizeof(ws->name));
	err = kobject_init_and_add(&ws->kobj, &device_type, devices_dir,
				   ws->name);
	if (err)
		return err;

	ws2801_init_clear(ws);

	ws2801_set_refresh_rate(ws, refresh_rate);

	platform_set_drvdata(pdev, ws);

	return 0;
}

static const struct of_device_id of_ws2801_match[] = {
	{ .compatible = DRIVER_NAME, },
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

	err = ws2801_sysfs_init(ws2801_dev);
	if (err)
		goto dev_out;

	err = platform_driver_register(&ws2801_driver);
	if (err)
		goto sysfs_unreg;

	return 0;

sysfs_unreg:
	ws2801_sysfs_exit(ws2801_dev);

dev_out:
	root_device_unregister(ws2801_dev);

	return err;
}

static void __exit ws2801_module_exit(void)
{
	platform_driver_unregister(&ws2801_driver);
	ws2801_sysfs_exit(ws2801_dev);
	root_device_unregister(ws2801_dev);
}

module_init(ws2801_module_init);
module_exit(ws2801_module_exit);

MODULE_AUTHOR("Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>");
MODULE_LICENSE("GPL");
