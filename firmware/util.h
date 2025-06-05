// util.h
// Copyright (c) 2023-2024 Daniel Cliche
// SPDX-License-Identifier: MIT

#ifndef __UTIL_H__
#define __UTIL_H__

unsigned char recv_byte(bool *avail);
void send_byte(unsigned char c);
void send_str(const char *str);

#endif
