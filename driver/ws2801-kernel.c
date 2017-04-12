#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include "ws2801-common.h"

#define WS2801_SYSFS "/sys/devices/ws2801/devices/"

#define WS2801_SYSFS_COMMIT "commit"
#define WS2801_SYSFS_NUM_LEDS "num_leds"
#define WS2801_SYSFS_REFRESH_RATE "refresh_rate"
#define WS2801_SYSFS_SET_RAW "set_raw"

struct ws2801_kernel {
	int fd_commit;
	int fd_refresh_rate;
	int fd_set_raw;
	int fd_num_leds;
	struct __attribute__((packed)) led *leds_packed;
};

static void ws2801_kernel_commit(struct ws2801_driver *ws_driver)
{
	struct ws2801_kernel *ws = ws_driver->drv_data;
	unsigned int i;
	struct __attribute__((packed)) led *dst = ws->leds_packed;
	struct led *src = ws_driver->leds;

	pthread_mutex_lock(&ws_driver->data_lock);

	for (i = 0; i < ws_driver->num_leds; i++, dst++, src++) {
		dst->r = src->r;
		dst->g = src->g;
		dst->b = src->b;
	}

	pthread_mutex_unlock(&ws_driver->data_lock);

	if (write(ws->fd_set_raw, ws->leds_packed,
		  ws_driver->num_leds * sizeof(*dst)) == -1) {
		fprintf(stderr, "ws2801: error during set_raw\n");
		exit(-errno);
	}

	if (write(ws->fd_commit, "", 1) == -1) {
		fprintf(stderr, "ws2801: error during commit\n");
		exit(-errno);
	}
}

static int ws2801_kernel_set_refresh_rate(struct ws2801_driver *ws_driver,
					  unsigned int refresh_rate)
{
	struct ws2801_kernel *ws = ws_driver->drv_data;
	char buffer[16];
	int bytes;

	bytes = snprintf(buffer, sizeof(buffer), "%u\n", refresh_rate);
	if (write(ws->fd_refresh_rate, buffer, bytes) == -1)
		return -errno;

	return 0;
}

static inline void __close_handle(int handle)
{
	if (handle > 0)
		close(handle);
}

static void __ws2801_kernel_free(struct ws2801_kernel *ws)
{
	__close_handle(ws->fd_commit);
	__close_handle(ws->fd_refresh_rate);
	__close_handle(ws->fd_set_raw);
	__close_handle(ws->fd_num_leds);

	if (ws->leds_packed)
		free(ws->leds_packed);

	free(ws);
}

static void ws2801_kernel_free(struct ws2801_driver *ws_driver)
{
	struct ws2801_kernel *ws = ws_driver->drv_data;

	__ws2801_kernel_free(ws);

	ws2801_free(ws_driver);
}

static int ws2801_kernel_set_num_leds(struct ws2801_driver *ws_driver,
				      unsigned int num_leds)
{

	struct ws2801_kernel *ws = ws_driver->drv_data;
	char buffer[16];
	int bytes;

	ws_driver->num_leds = num_leds;

	bytes = snprintf(buffer, sizeof(buffer), "%u\n", num_leds);
	if (write(ws->fd_num_leds, buffer, bytes) == -1)
		return -errno;

	return 0;
}

int ws2801_kernel_init(unsigned int num_leds, const char *device_name,
		       struct ws2801_driver *ws_driver)
{
	struct ws2801_kernel *ws;
	char buffer[1024];
	int err;

	ws = calloc(1, sizeof(*ws));
	if (!ws)
		return -ENOMEM;

	ws->leds_packed = malloc(num_leds * sizeof(*ws->leds_packed));
	if (!ws->leds_packed) {
		err = -ENOMEM;
		goto free_out;
	}

	err = ws2801_init(ws_driver, num_leds);
	if (err)
		goto free_out;

	ws_driver->drv_data = ws;

#define OPEN_SYSFS_ATTRIBUTE(__fd, __attribute) \
	snprintf(buffer, sizeof(buffer), WS2801_SYSFS "%s/" __attribute, \
		 device_name); \
	__fd = open(buffer, O_WRONLY); \
	if (__fd < 0) { \
		fprintf(stderr, "unable to open %s\n", buffer); \
		err = -errno; \
		goto free_out; \
	}

	OPEN_SYSFS_ATTRIBUTE(ws->fd_commit, WS2801_SYSFS_COMMIT);
	OPEN_SYSFS_ATTRIBUTE(ws->fd_refresh_rate, WS2801_SYSFS_REFRESH_RATE);
	OPEN_SYSFS_ATTRIBUTE(ws->fd_set_raw, WS2801_SYSFS_SET_RAW);
	OPEN_SYSFS_ATTRIBUTE(ws->fd_num_leds, WS2801_SYSFS_NUM_LEDS);
#undef OPEN_SYSFS_ATTRIBUTE

	err = ws2801_kernel_set_num_leds(ws_driver, num_leds);
	if (err)
		goto free_out;

	ws_driver->set_auto_commit = ws2801_set_auto_commit;
	ws_driver->clear = ws2801_clear;
	ws_driver->commit = ws2801_kernel_commit;
	ws_driver->set_refresh_rate = ws2801_kernel_set_refresh_rate;
	ws_driver->set_led = ws2801_set_led;
	ws_driver->set_leds = ws2801_set_leds;
	ws_driver->free = ws2801_kernel_free;

	return 0;

free_out:
	__ws2801_kernel_free(ws);
	return err;
}
