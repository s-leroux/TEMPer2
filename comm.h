#ifndef TEMPER_COMM_H
#define TEMPER_COMM_H

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

struct TemperData {
	float value;
	enum Unit {
		TEMPER_UNAVAILABLE,	/* unavailable data */
		TEMPER_REL_HUM, 	/* relative humidity (in %) */
		TEMPER_ABS_TEMP,	/* absolute temperature  (in °C) */
	} unit;
};
typedef struct TemperData TemperData;

#define TemperUnitToString(unit) ( (unit == TEMPER_ABS_TEMP) ? "°C" : \
				   (unit == TEMPER_REL_HUM) ? "%RH" : \
				   "" \
	)

struct Temper {
        struct usb_device *device;
        usb_dev_handle *handle;
        int debug;
        int timeout;
        const struct Product    *product;
};
typedef struct Temper Temper;


typedef int (*TemperConvertFct)(Temper*, int16_t word, TemperData* dst);

struct Product {
        uint16_t                vendor;
        uint16_t                id;
        const char              *name;
        TemperConvertFct        convert[2]; /* Arbitrary limit ? */
};

Temper *TemperCreateFromDeviceNumber(int deviceNum, int timeout, int debug);
void TemperFree(Temper *t);

int TempterGetOtherStuff(Temper *t, char *buf, int length);

int TemperSendCommand8(Temper *t, int a, int b, int c, int d, int e, int f, int g, int h);
int TemperSendCommand2(Temper *t, int a, int b);
int TemperGetData(Temper *t, TemperData *data, unsigned int count);

int TemperInterruptRead(Temper* t, unsigned char *buf, unsigned int len);

int TemperGetSerialNumber(Temper* t, char* buf, unsigned int len);

#endif

