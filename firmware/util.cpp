// util.cpp
// Copyright (c) 2023-2024 Daniel Cliche
// SPDX-License-Identifier: MIT

#include "Arduino.h"

#include "util.h"

unsigned char recv_byte(bool *avail)
{
    unsigned char c = 0;
    *avail = false;
    if (Serial.available()) {
        c = Serial.read();
        *avail = true;
    }
    return c;
}

void send_byte(unsigned char c)
{
    Serial.write(c);
}

void send_str(const char *str)
{
    while(*str) {
        send_byte(*str);
        str++;
    }
}
