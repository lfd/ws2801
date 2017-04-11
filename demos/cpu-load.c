/*
 * ws2801 - WS2801 LED driver running in Linux userspace
 *
 * Copyright (c) - Marvin Vierling, 2017
 * Copyright (c) - Ralf Ramsauer, 2017
 * Copyright (c) - Tobias Kottwitz, 2017
 *
 * Authors:
 *   Marvin Vierling <marvin.vierling@st.oth-regensburg.de>
 *   Ralf Ramsauer <ralf.ramsauer@oth-regensburg.de>
 *   Tobias Kottwitz <tobias.kottwitz@st.oth-regensburg.de>
 *
 * This work is licensed under the terms of the GNU GPL, version 2.  See
 * the COPYING file in the top-level directory.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

#include <ws2801.h>

#define NUM_PIXELS 10
#define DEVICE "gpiochip0"
#define GPIO_CLK 160
#define GPIO_DO 162

#define PROCSTATFILE "/proc/stat"
#define NUM_PARAMS 8

struct cpu_stats {
	unsigned long long int user;
	unsigned long long int nice;
	unsigned long long int system;
	unsigned long long int idle;
	unsigned long long int iowait;
	unsigned long long int irq;
	unsigned long long int softirq;
	unsigned long long int steal;
};

static struct pixel pixels[NUM_PIXELS];

int get_cpu_stats(struct cpu_stats *stats)
{
	char buffer[256];
	int err = -1;
	FILE *file;

	file = fopen(PROCSTATFILE, "r");
	if (file == NULL)
		return -1;

	if (fgets(buffer, sizeof(buffer), file) == NULL) 
	{
		fprintf(stderr, "fgets");
		goto out;
	}

	err = sscanf(buffer,
		"cpu  %16llu %16llu %16llu %16llu %16llu %16llu %16llu %16llu",
		&(stats->user),
		&(stats->nice),
		&(stats->system),
		&(stats->idle),
		&(stats->iowait),
		&(stats->irq),
		&(stats->softirq),
		&(stats->steal)
	);
	if (err < NUM_PARAMS)
		goto out;

	err = 0;
out:
	fclose(file);
	return err;

}

static unsigned long long int inline sum_usage(struct cpu_stats *stats)
{
	return stats->user + stats->nice + stats->system +
		stats->irq + stats->softirq + stats->steal;
}

static unsigned long long int inline sum_idle(struct cpu_stats *stats)
{
	return stats->idle + stats->iowait;
}

int get_cpu_usage(struct cpu_stats *stats_prev, double *result)
{
	unsigned long long int delta_idle, delta_usage;
	struct cpu_stats stats_curr;

	if (get_cpu_stats(&stats_curr) < 0)
		return -1;

	delta_usage = sum_usage(&stats_curr) - sum_usage(stats_prev);
	delta_idle = sum_idle(&stats_curr) - sum_idle(stats_prev);

	*result = (double)delta_usage / (delta_usage + delta_idle);
	if (*result > 1)
		*result = 1;

	*stats_prev = stats_curr;

	return 0;
}

int main(void)
{
	int err, num, i;
	double red, green;
	struct ws2801 *ws;
	struct cpu_stats stats;
	double usage;

	err = ws2801_init(NUM_PIXELS, DEVICE, GPIO_CLK, GPIO_DO, &ws);
	if (err) {
		fprintf(stderr, "initialising ws2801: %s\n", strerror(-err));
		return err;
	}

	err = get_cpu_stats(&stats);
	if (err)
		fprintf(stderr, "Unable to get CPU stats\n");

	/* wait a bit between the first and second cpu usage measurement */
	usleep(10000);

	for (;;) {
		err = get_cpu_usage(&stats, &usage);
		if (err) {
			fprintf(stderr, "Unable to get CPU stats\n");
			break;
		}

		printf("\r%f ", usage);
		fflush(stdout);

		red = usage * 255;
		green = 255 - red;

		num = usage * 10;

		memset(pixels, 0, sizeof(pixels));

		for (i = 0; i < num; i++) {
			pixels[i].r = red;
			pixels[i].g = green;
		}

		err = ws2801_set_pixels(ws, pixels, 0, NUM_PIXELS);
		if (err) {
			fprintf(stderr, "set pixel\n");
			break;
		}
		err = ws2801_update(ws);
		if (err) {
			fprintf(stderr, "ws2801: error updating pixels\n");
			break;
		}

		usleep(200000);
	}

	ws2801_free(ws);
	return 0;
}
