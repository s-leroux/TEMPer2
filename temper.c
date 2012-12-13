#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <usb.h>
#include <errno.h>

#include "comm.h"

#define USB_TIMEOUT 1000	/* milliseconds */

int
main(void)
{
	Temper *t;
	char buf[256];
	int i, ret;

	usb_set_debug(0);
	usb_init();
	usb_find_busses();
	usb_find_devices();

	t = TemperCreateFromDeviceNumber(0, USB_TIMEOUT, 1);
	if(!t) {
		perror("TemperCreate");
		exit(-1);
	}

/*
	TemperSendCommand(t, 10, 11, 12, 13, 0, 0, 2, 0);
	TemperSendCommand(t, 0x43, 0, 0, 0, 0, 0, 0, 0);
	TemperSendCommand(t, 0, 0, 0, 0, 0, 0, 0, 0);
	TemperSendCommand(t, 0, 0, 0, 0, 0, 0, 0, 0);
	TemperSendCommand(t, 0, 0, 0, 0, 0, 0, 0, 0);
	TemperSendCommand(t, 0, 0, 0, 0, 0, 0, 0, 0);
	TemperSendCommand(t, 0, 0, 0, 0, 0, 0, 0, 0);
	TemperSendCommand(t, 0, 0, 0, 0, 0, 0, 0, 0);
*/

	printf("[\n");
//	TemperSendCommand2(t, 0x01,0x01);
//	TemperSendCommand8(t, 0x48, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
	TemperSendCommand8(t, 0x01, 0x80, 0x33, 0x01, 0x00, 0x00, 0x00, 0x00);
	if (0) {
		unsigned char buf[8];
		TemperInterruptRead(t, buf, sizeof(buf));
	}
	else {
		struct TemperData data;
		ret = TemperGetData(t,&data);
		printf("ret = %d; tempA = %f°C tempB = %f°C\n",
			ret,
			data.tempA,
			data.tempB);
	}

	printf("]\n");
#if 0
	bzero(buf, 256);
	ret = TemperGetOtherStuff(t, buf, 256);
	printf("Other Stuff (%d bytes):\n", ret);
	for(i = 0; i < ret; i++) {
		printf(" %02x", buf[i] & 0xFF);
		if(i % 16 == 15) {
			printf("\n");
		}
	}
	printf("\n");

	for(;;) {
		float tempc;

		if(TemperGetTemperatureInC(t, &tempc) < 0) {
			perror("TemperGetTemperatureInC");
			exit(1);
		}
		printf("temperature %.2fF %.2fC\n", (9.0 / 5.0 * tempc + 32.0),
		       tempc);
		sleep(10);
	}
#endif
	return 0;
}

