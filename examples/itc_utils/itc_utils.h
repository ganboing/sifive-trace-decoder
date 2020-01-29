/* Copyright 2020 SiFive, Inc */
/* SPDX-License-Identifier: Apache-2.0 */

#ifndef ITC_UTILS_H
#define ITC_UTILS_H

int itc_set_channel(int channel);
int itc_puts(const char *f);
int itc_printf(const char *f, ... );

#endif // ITC_UTILS_H
