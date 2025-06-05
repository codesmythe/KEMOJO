// ps2_mouse.h
// Copyright (c) 2023 Daniel Cliche
// SPDX-License-Identifier: MIT

#ifndef PS2_MOUSE_H
#define PS2_MOUSE_H

#include <stdint.h>

class PS2Mouse
{
public:
    PS2Mouse();

    static void begin(int clk_pin, int data_pin);
    static uint8_t read(bool *avail, bool *buffer_overflow);
};

#endif // PS2_MOUSE_H
