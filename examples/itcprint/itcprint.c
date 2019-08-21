/* Copyright 2019 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#define	baseAddress	0x20007000

// Register Offsets

#define Offset_teControl		0x0000
#define Offset_teImpl			0x0004
#define	Offset_teSinkBase		0x0010
#define Offset_teSinkBaseHigh	0x0014
#define Offset_teSinkLimit		0x0018
#define Offset_teSinkWP			0x001c
#define Offset_teSinkRP			0x0020
#define	Offset_teSinkData		0x0024
#define	Offset_tsControl		0x0040
#define Offset_tsLower			0x0044
#define Offset_tsUpper			0x0048
#define Offset_xtiControl		0x0050
#define	Offset_xtoControl		0x0054
#define	Offset_wpControl		0x0058
#define Offset_itcTraceEnable	0x0060
#define	Offset_itcTrigEnable	0x0064
#define	Offset_itcStimulus		0x0080
#define	Offset_atbSink			0x0e00
#define Offset_pibSink			0x0f00

// Register Address and types

#define teControl		((uint32_t*)(baseAddress+Offset_teControl))
#define teImpl			((uint32_t*)(baseAddress+Offset_teImpl))
#define	teSinkBase		((uint32_t*)(baseAddress+Offset_teSinkBase))
#define teSinkBaseHigh	((uint32_t*)(baseAddress+Offset_teSinkBaseHigh))
#define teSinkLimit		((uint32_t*)(baseAddress+Offset_teSinkLimit))
#define teSinkWP		((uint32_t*)(baseAddress+Offset_teSinkWP))
#define teSinkRP		((uint32_t*)(baseAddress+Offset_teSinkRP))
#define	teSinkData		((uint32_t*)(baseAddress+Offset_teSinkData))
#define	tsControl		((uint32_t*)(baseAddress+Offset_tsControl))
#define tsLower			((uint32_t*)(baseAddress+Offset_tsLower))
#define tsUpper			((uint32_t*)(baseAddress+Offset_tsUpper))
#define xtiControl		((uint32_t*)(baseAddress+Offset_xtiCtonrol))
#define	xtoControl		((uint32_t*)(baseAddress+Offset_xtoCtonrol))
#define	wpControl		((uint32_t*)(baseAddress+Offset_wpControl))
#define itcTraceEnable	((uint32_t*)(baseAddress+Offset_itcTraceEnable))
#define	itcTrigEnable	((uint32_t*)(baseAddress+Offset_itcTrigEnable))
#define	itcStimulus		((uint32_t*)(baseAddress+Offset_itcStimulus))
#define	atbSink			((uint32_t*)(baseAddress+Offset_atbSink))
#define pibSink			((uint32_t*)(baseAddress+Offset_pibSink))

static int enable_itc(int reg)
{
	if ((reg < 0) || (reg > 31)) {
		return 1;
	}

	*itcTraceEnable |= (1 << reg);

	return 0;
}

static int init_itc(int reg)
{
	return enable_itc(reg);
}

static inline void write_itc(uint32_t data)
{
	itcStimulus[0] = data;
}

static inline void write_itc_uint8(uint8_t data)
{
	uint8_t *itc_uint8 = (uint8_t*)&itcStimulus[0];
	itc_uint8[3] = data;
}

static inline void write_itc_uint16(uint16_t data)
{
	uint16_t *itc_uint16 = (uint16_t*)&itcStimulus[0];
	itc_uint16[1] = data;
}

int itcprintf(const char *f, ... )
{
	char buffer[256];
	va_list args;
	int rc;

	va_start(args, f);
	rc = vsnprintf(buffer,sizeof buffer, f, args);
	va_end(args);

	itcprintstr(buffer);
	return rc;
}

int itcprintstr(const char *f)
{
	static int inited = 0;

	if (inited == 0) {
		int rc;

		rc = init_itc(0);
		if (rc != 0) {
			return 0;
		}

		inited = 1;
	}

	int rc = strlen(f);

	int words = (rc/4)*4;
	int bytes = rc & 0x03;
	int i;
	uint16_t a;

    for (i = 0; i < words; i += 4) {
		write_itc(*(uint32_t*)(buffer+i));
	}

    switch (bytes) {
    case 0:
    	break;
    case 1:
    	write_itc_uint8(*(uint8_t*)(buffer+i));
    	break;
    case 2:
    	write_itc_uint16(*(uint16_t*)(buffer+i));
    	break;
    case 3:
    	a = *(uint16_t*)(buffer+i);
    	write_itc_uint16(a);

    	a = ((uint16_t)(*(uint8_t*)(buffer+i+2)));
    	write_itc_uint8((uint8_t)a);
    	break;
    }

	return rc;
}
