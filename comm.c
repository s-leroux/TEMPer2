#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <usb.h>
#include <errno.h>

/*
 * Temper.c by Robert Kavaler (c) 2009 (relavak.com)
 * All rights reserved.
 *
 * Modified by Sylvain Leroux (c) 2012 (sylvain@chicoree.fr)
 *
 * Temper driver for linux. This program can be compiled either as a library
 * or as a standalone program (-DUNIT_TEST). The driver will work with some
 * TEMPer usb devices from RDing (www.PCsensor.com).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Robert kavaler BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 */

#include "comm.h"

static int TEMPer2V13ToTemperature(Temper*,int16_t word, TemperData* dst);
static int TEMPerHUMToTemperature(Temper*,int16_t word, TemperData* dst);
static int TEMPerHUMToHumidity(Temper*,int16_t word, TemperData* dst);
static int TemperUnavailable(Temper*,int16_t word, TemperData* dst);

static const struct Product ProductList[] = {
/*
	Not supported: commands are different
	{
		0x1130, 0x660c,
		"Original RDing TEMPer"
	},
*/
	{
		/* Analog Device ADT75 (or similar) based device */
		/* with two temperature sensors (internal & external) */
		0x0c45, 0x7401,
		"RDing TEMPer2V1.3",
		{
			TEMPer2V13ToTemperature,
			TEMPer2V13ToTemperature,
		},
	},
	{
		/* Sensirion SHT1x based device */
		/* with internal humidity & temperature sensor */
		0x0c45, 0x7402,
		"RDing TEMPerHumiV1.1",
		{
			TEMPerHUMToTemperature,
			TEMPerHUMToHumidity,
		},
	},
};
static const unsigned ProductCount = sizeof(ProductList)/sizeof(struct Product);

Temper *
TemperCreate(struct usb_device *dev, int timeout, int debug, const struct Product* product)
{
	Temper *t;
	int ret;

	if (debug) {
		printf("Temper device %s (%04x:%04x)\n",
			product->name,
			product->vendor,
			product->id);
	}

	t = calloc(1, sizeof(*t));
	t->device = dev;
	t->debug = debug;
	t->product = product;
	t->timeout = timeout;
	t->handle = usb_open(t->device);
	if(!t->handle) {
		free(t);
		return NULL;
	}
	if(t->debug) {
		printf("Trying to detach kernel driver\n");
	}

	ret = usb_detach_kernel_driver_np(t->handle, 0);
	if(ret) {
		if(errno == ENODATA) {
			if(t->debug) {
				printf("Device already detached\n");
			}
		} else {
			if(t->debug) {
				printf("Detach failed: %s[%d]\n",
				       strerror(errno), errno);
				printf("Continuing anyway\n");
			}
		}
	} else {
		if(t->debug) {
			printf("detach successful\n");
		}
	}
	ret = usb_detach_kernel_driver_np(t->handle, 1);
	if(ret) {
		if(errno == ENODATA) {
			if(t->debug)
				printf("Device already detached\n");
		} else {
			if(t->debug) {
				printf("Detach failed: %s[%d]\n",
				       strerror(errno), errno);
				printf("Continuing anyway\n");
			}
		}
	} else {
		if(t->debug) {
			printf("detach successful\n");
		}
	}

	if(usb_set_configuration(t->handle, 1) < 0 ||
	   usb_claim_interface(t->handle, 0) < 0 ||
	   usb_claim_interface(t->handle, 1)) {
		usb_close(t->handle);
		free(t);
		return NULL;
	}
	return t;
}

Temper *
TemperCreateFromDeviceNumber(int deviceNum, int timeout, int debug)
{
	struct usb_bus *bus;
	int n;

	n = 0;
	for(bus=usb_get_busses(); bus; bus=bus->next) {
	    struct usb_device *dev;

	    for(dev=bus->devices; dev; dev=dev->next) {
		if(debug) {
			printf("Found device: %04x:%04x\n",
			       dev->descriptor.idVendor,
			       dev->descriptor.idProduct);
		}
		for(unsigned i = 0; i < ProductCount; ++i) {	
			if(dev->descriptor.idVendor == ProductList[i].vendor &&
			   dev->descriptor.idProduct == ProductList[i].id) {
				if(debug) {
				    printf("Found deviceNum %d\n", n);
				}
				if(n == deviceNum) {
					return TemperCreate(dev, timeout,
							 debug, 
							 &ProductList[i]);
				}
				n++;
			}
		}
	    }
	}
	return NULL;
}

void
TemperFree(Temper *t)
{
	if(t) {
		if(t->handle) {
			usb_close(t->handle);
		}
		free(t);
	}
}

int
TemperSendCommand8(Temper *t, int a, int b, int c, int d, int e, int f, int g, int h)
{
	unsigned char buf[8+8*8];
	int ret;

	bzero(buf, sizeof(buf));
	buf[0] = a;
	buf[1] = b;
	buf[2] = c;
	buf[3] = d;
	buf[4] = e;
	buf[5] = f;
	buf[6] = g;
	buf[7] = h;

	if(t->debug) {
		printf("sending bytes %02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x (buffer len = %d)\n",
		       a, b, c, d, e, f, g, h, sizeof(buf));
	}

	ret = usb_control_msg(t->handle, 0x21, 9, 0x200, 0x01,
			    (char *) buf, sizeof(buf), t->timeout);

	if(ret != sizeof(buf)) {
		perror("usb_control_msg failed");
		return -1;
	}
	return 0;
}

int
TemperSendCommand2(Temper *t, int a, int b)
{
	unsigned char buf[8+8*8];
	int ret;

	bzero(buf, sizeof(buf));
	buf[0] = a;
	buf[1] = b;

	if(t->debug) {
		printf("sending bytes %02x, %02x (buffer len = %d)\n",
		       a, b, sizeof(buf));
	}

	ret = usb_control_msg(t->handle, 0x21, 9, 0x201, 0x00,
			    (char *) buf, sizeof(buf), t->timeout);

	if(ret != sizeof(buf)) {
		perror("usb_control_msg failed");
		return -1;
	}
	return 0;
}

int TemperInterruptRead(Temper* t, unsigned char *buf, unsigned int len) {
	int ret;

	if (t->debug) {
		printf("interrupt read\n");
	}

	ret = usb_interrupt_read(t->handle, 0x82, (char*)buf, len, t->timeout);
	if(t->debug) {
		printf("receiving %d bytes\n",ret);
		for(int i = 0; i < ret; ++i) {
			printf("%02x ", buf[i]);
			if ((i+1)%8 == 0) printf("\n");
		}
		printf("\n");
        }

	return ret;
}

static int TEMPer2V13ToTemperature(Temper* t, int16_t word, TemperData* dst) {
#if 0
	word += t->offset; /* calibration value */
#endif

	dst->value = ((float)word) * (125.0 / 32000.0);
	dst->unit = TEMPER_ABS_TEMP;

	return 0;
}

static int TEMPerHUMToTemperature(Temper* t, int16_t word, TemperData* dst) {
#if 0
	word += t->offset; /* calibration value */
#endif

	/* assuming Vdd = 5V (from USB) and 14bit precision */
	dst->value = ((float)word) * (0.01) + -40.1;
	dst->unit = TEMPER_ABS_TEMP;

	return 0;
}

static int TEMPerHUMToHumidity(Temper* t, int16_t word, TemperData* dst) {
	const float rh = (float)word;

	/* assuming 12 bits readings */
	const float c1 = -2.0468;
	const float c2 = .0367;
	const float c3 = -1.5955e-6;

	dst->value = c1 + c2*rh + c3*rh*rh;
	dst->unit = TEMPER_REL_HUM;

	return 0;
}

static int TemperUnavailable(Temper* t, int16_t word, TemperData* dst) {
	dst->value = 0.0;
	dst->unit = TEMPER_UNAVAILABLE;

	return 0;
}

int
TemperGetData(Temper *t, struct TemperData *data, unsigned int count) {
	unsigned char buf[8];
	int ret = TemperInterruptRead(t, buf, sizeof(buf));

	for(int i = 0; i < count; ++i) {
		if ((2*i+3) < ret) {
			int16_t word = ((int8_t)buf[2*i+2] << 8) | buf[2*i+3];
			t->product->convert[i](t, word, &data[i]);
		}
		else {
			TemperUnavailable(t, 0, &data[i]);
		}
	}
	
	return ret;
}

int TemperGetSerialNumber(Temper* t, char* buf, unsigned int len) {
	if (len == 0)
		return -EINVAL;

	if (t->device->descriptor.iSerialNumber == 0) {
		buf[0] = 0;
		return -ENOENT;
	}


	return usb_get_string_simple(t->handle,
					t->device->descriptor.iSerialNumber,
					buf, len);
}

