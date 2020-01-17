/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef ITC_UTILS_H
#define ITC_UTILS_H

int itc_puts(const char *f);
int itc_fputs(int channel,const char *f);
int itc_printf(const char *f, ... );
int itc_fprintf(int channel,const char *f, ... );

#endif // ITC_UTILS_H
