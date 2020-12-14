// SPDX-License-Identifier: GPL-2.0
/*
 * Real Time Clock Driver Test Program
 *
 * Copyright (c) 2018 Alexandre Belloni <alexandre.belloni@bootlin.com>
 */
#define CONFIG_RTC_100TH_SEC y

#include <errno.h>
#include <fcntl.h>
#include <linux/rtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "../kselftest_harness.h"

#define NUM_UIE 3
#define ALARM_DELTA 3

#define PCF85263A_TSR2_FLAG_BIT_MASK    (0x01 << 1)  /* Time-stamp 2(power fail time) event flag bit mask */

struct power_fail_data
{
    unsigned char flag_status;
    unsigned long power_fail_tm;
};

#define IOCTL_PCF85263A_GET_POWER_FAIL_TM _IOR('p', 0xa0, struct power_fail_data *) /** Read system power fail time */

static char *rtc_file = "/dev/rtc1";

FIXTURE(rtc) {
	int fd;
};

FIXTURE_SETUP(rtc) {
	self->fd = open(rtc_file, O_RDONLY);
	ASSERT_NE(-1, self->fd);
}

FIXTURE_TEARDOWN(rtc) {
	close(self->fd);
}



TEST_F(rtc, alarm_wkalm_set) {
	struct timeval tv = { .tv_sec = ALARM_DELTA + 2 };
	struct rtc_wkalrm alarm = { 0 };
	struct rtc_time tm;
	unsigned long data;
	fd_set readfds;
	time_t secs, new;
	int rc;
	

   struct power_fail_data pfd;
   time_t time = 0;

	rc = ioctl(self->fd, IOCTL_PCF85263A_GET_POWER_FAIL_TM, &pfd);
	//ASSERT_NE(-1, rc);

	 printf("Read in powerfail time valid %d, time: %lu\n",
			(pfd.flag_status & PCF85263A_TSR2_FLAG_BIT_MASK) != 0, pfd.power_fail_tm );
			
	 if ((pfd.flag_status & PCF85263A_TSR2_FLAG_BIT_MASK) != 0)
	 {
		time = (time_t) pfd.power_fail_tm;
	 }


	rc = ioctl(self->fd, RTC_RD_TIME, &alarm.time);
	printf("100th  is %d , rc = %d\n", alarm.time.tm_100th_sec,rc );
	//ASSERT_NE(-1, rc);

	printf("After RTC_RD_TIME.\n");

	alarm.time.tm_100th_sec = 0;
	alarm.time.tm_sec = alarm.time.tm_sec + 10;
	
	alarm.enabled = 1;

	rc = ioctl(self->fd, RTC_WKALM_SET, &alarm);
	if (rc == -1) {
		ASSERT_EQ(EINVAL, errno);
		TH_LOG("skip alarms are not supported.");
		return;
	}

	rc = ioctl(self->fd, RTC_WKALM_RD, &alarm);
	//printf("rc = %d\n", rc );
	//ASSERT_NE(-1, rc);

	TH_LOG("Alarm time now set to %02d/%02d/%02d %02d:%02d:%02d.  100th=%02d",
	       alarm.time.tm_mday, alarm.time.tm_mon + 1,
	       alarm.time.tm_year + 1900, alarm.time.tm_hour,
	       alarm.time.tm_min, alarm.time.tm_sec, alarm.time.tm_100th_sec);
}


static void __attribute__((constructor))
__constructor_order_last(void)
{
	if (!__constructor_order)
		__constructor_order = _CONSTRUCTOR_ORDER_BACKWARD;
}

int main(int argc, char **argv)
{
	
#ifdef CONFIG_RTC_100TH_SEC
	printf("CONFIG_RTC_100TH_SEC is set\n");
#endif	
	switch (argc) {
	case 2:
		rtc_file = argv[1];
		/* FALLTHROUGH */
	case 1:
		break;
	default:
		fprintf(stderr, "usage: %s [rtcdev]\n", argv[0]);
		return 1;
	}

	return test_harness_run(argc, argv);
}
